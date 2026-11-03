/**********************************************************************
 * little_cache.h
 *
 * Copyright 2025 W. Greg Visotski
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *         limitations under the License.
 **********************************************************************/

#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <stack>
#include <type_traits>


/**
 * Reset policy that leaves returned items exactly as they were (the default).
 */
struct little_cache_keep{
    template<typename T>
    void operator()(T& /*item*/) const noexcept {}
};

/**
 * Reset policy that calls clear() on each returned item.  Items come back empty but keep their memory, so a
 * std::vector keeps its capacity.  NOTE: the old contents are not erased from that memory, only made invisible.
 */
struct little_cache_clear{
    template<typename T>
        requires requires(T& item) { item.clear(); }
    void operator()(T& item) const noexcept {
        item.clear();
    }
};


/**
 * This is a simple auto-recycler of things.  If you ask the little_cache to construct something for you, it
 * will provide you a shared_ptr to that thing.  The shared pointer has a deleter that will automatically
 * return the item to this little_cache instead of actually deleting it.  That way, subsequent calls will
 * return an already-constructed item.
 *
 * The second template parameter allows you to have multiple independent caches
 * for the same type, without me to have to manage that.
 *
 * NOTE1: Items returned via get() may be fully constructed, setup, and contain data.
 * NOTE2: Once items are collected via shared_ptr, they are not actually deleted.  Any data within those objects
 *        will be visible to some future call to get().  Don't leave anything sensitive in there.
 * NOTE3: There will only be one instance of this cache per type!  It is singleton, per template type.
 * NOTE4: All functions are safe to call from any number of threads at once.
 * NOTE5: The third template parameter is a reset policy, called on every item as it is returned to the cache (and
 *        before it is deleted, if the cache is full).  little_cache_keep (the default) leaves items as they were;
 *        little_cache_clear calls clear() on them.  Your own policy is any default-constructible type with a
 *        noexcept "void operator()(TYPE&) const".  Caches with different policies are independent caches.
 *
 * example usage:
 * typedef std::vector<std::complex<float> > ITEM_TYPE;
 * typedef std::shared_ptr<ITEM_TYPE> ITEM;
 * <little_cache is empty>
 * {
 *    ITEM myItem = little_cache<ITEM_TYPE>::get();
 *    <little_cache is still empty>
 * }
 * <little_cache contains one item>

 * @tparam TYPE  - the type of objects to be cached
 * @tparam X     - optional cache identifier - so you can have multiple independent caches of the same type
 * @tparam RESET - optional reset policy, applied to items as they are returned (see NOTE5)
 */
template<typename TYPE, int X = 0, typename RESET = little_cache_keep>
class little_cache{
public:
    static_assert(std::is_default_constructible_v<TYPE>,  "little_cache needs to be able to construct new items");
    static_assert(!std::is_array_v<TYPE>,                  "little_cache can't cache arrays; use std::array or std::vector");
    static_assert(std::is_default_constructible_v<RESET>, "the reset policy must be default constructible");
    static_assert(std::is_nothrow_invocable_v<const RESET&, TYPE&>,
                  "the reset policy must be callable as policy(item) and noexcept: it runs inside a destructor");

    /**
     * The most items kept at rest per cache.  Items returned while the cache is full are deleted.
     *
     * TODO -> make the size limit something better
     */
    static constexpr std::size_t MAX_CACHED_ITEMS{1024};

    /**
     * No copying allowed
     */
    little_cache(const little_cache&) = delete;
    /**
     * No copying allowed
     */
    const little_cache& operator=(const little_cache&) = delete;

    /**
     * Get an item from the cache if there is one available, or construct one if not.  These
     * items will automatically be returned to the cache when std::shared_ptr decides to garbage
     * collect it.
     *
     * Items returned may be brand new, or may be used.  It is your responsibility to properly configure
     * the item when you get it, and remove private stuff before it is allowed to be recycled.
     *
     * @return std::shared_ptr<TYPE> - an item for you to use - it may be new or used
     * @throws whatever TYPE's default constructor throws, or std::bad_alloc
     */
    static std::shared_ptr<TYPE> get(){
        //try to get a used item
        TYPE* t{little_cache<TYPE,X,RESET>::tryGetFromCache()};
        if(nullptr == t){
            //no used item available, make a new one
            t = new TYPE();
        }

        //make a deleter.  It runs inside std::shared_ptr's destructor, so it must not throw: if the item can't
        //be put back for any reason, it is simply deleted.
        auto deleter = [](TYPE* used) noexcept {
            const RESET reset{};
            reset(*used);

            bool returned{false};
            try {
                returned = little_cache<TYPE,X,RESET>::tryReturn(used);
            } catch (...) {
                returned = false;
            }
            if(!returned){
                delete used;
            }
        };

        //put the two together (if this throws, std::shared_ptr calls the deleter for us)
        return std::shared_ptr<TYPE>(t, deleter);
    }

    /**
     * Destruct everything in the cache.  This has no effect on things produced by this cache that have not yet been
     * cleaned up.  In other words, things may appear in this cache unexpectedly after purging.
     */
    static void purge() {
        little_cache<TYPE,X,RESET>* me = little_cache<TYPE,X,RESET>::getCache();
        std::stack<TYPE*>      doomed;
        {
            std::lock_guard<std::mutex> lock(me->thingsLock);
            doomed.swap(me->things);
        }
        //destruct outside the lock, so a destructor that uses this cache can't deadlock
        while (!doomed.empty()) {
            delete doomed.top();
            doomed.pop();
        }
    }

    /**
     * This is a diagnostic function which returns the number of items in the cache awaiting use.  This does not
     * count the number of items currently in flight.
     * @return the number of items at rest awaiting use
     */
    static std::size_t numCachedItems() {
        little_cache<TYPE,X,RESET>* me = little_cache<TYPE,X,RESET>::getCache();
        std::lock_guard<std::mutex> lock(me->thingsLock);
        return me->things.size();
    }

private:
    std::stack<TYPE*> things;
    std::mutex        thingsLock;

    little_cache() = default;
    ~little_cache() = default;

    /**
     * Get the singleton instance.  It is created on first use (thread-safe, as for any function-local static) and is
     * deliberately never destroyed: a shared_ptr released during static destruction, after this cache would
     * otherwise have been destroyed, can still return its item safely.
     * @return a pointer to the singleton little_cache<TYPE,X,RESET>
     */
    static little_cache<TYPE,X,RESET>* getCache(){
        static little_cache<TYPE,X,RESET>* const instance{new little_cache<TYPE,X,RESET>()};
        return instance;
    }

    /**
     * Try to put an item back into the cache.
     * NOTE: If this returns false, you, the caller, are still the owner of the pointer.
     *
     * @param t - the pointer you wish to return
     * @return true if successfully cached, the pointer is mine and you can't have it back
     *         false if insertion failed, you keep that pointer
     */
    static bool tryReturn(TYPE* t){
        little_cache<TYPE,X,RESET>* me = little_cache::getCache();
        std::lock_guard<std::mutex> lock(me->thingsLock);
        if (me->things.size() < MAX_CACHED_ITEMS) {
            me->things.push(t);
            return true;
        }
        return false;
    }

    /**
     * Like the name implies, try to get a used item.  If none is available, I'll just return you a nullptr.
     * @return a used item, or nullptr
     */
    static TYPE* tryGetFromCache(){
        little_cache<TYPE,X,RESET>* me = little_cache::getCache();
        //always take the lock: checking empty() without it races with other threads pushing and popping
        std::lock_guard<std::mutex> lock(me->thingsLock);
        if(me->things.empty()){
            return nullptr;
        }

        //a used item exists - return it
        TYPE* t = me->things.top();
        me->things.pop();
        return t;
    }
};
