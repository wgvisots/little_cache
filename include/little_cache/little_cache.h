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
 * A simple recycler for objects that are expensive to construct.  get() hands you a std::shared_ptr to an object.
 * When the last copy of that shared_ptr goes away, the object is put back in this cache instead of being deleted,
 * and a later get() hands it out again.
 *
 * NOTE1: An object from get() may be brand new, or may be one that was used before and still holds its old data.
 * NOTE2: Recycled objects are not wiped unless a reset policy does it (see NOTE5).  Whatever you leave in an object
 *        will be visible to the next caller of get().  Don't leave anything sensitive in there.
 * NOTE3: There is one cache per combination of TYPE, ID and RESET.  Use different IDs for independent caches of the
 *        same type.
 * NOTE4: All functions are safe to call from any number of threads at once.
 * NOTE5: RESET is called on every object as it is returned to the cache (and before it is deleted, if the cache is
 *        full).  little_cache_keep (the default) leaves objects as they were; little_cache_clear calls clear() on
 *        them.  Your own policy is any default-constructible type with a noexcept "void operator()(TYPE&) const".
 * NOTE6: The cache and the objects at rest in it are never destroyed, even at program exit, so that a shared_ptr
 *        released during static destruction can still return its object safely.  Leak checkers report them as
 *        "still reachable", not as leaks.  Call purge() to free them earlier.
 *
 * Example:
 *     {
 *         std::shared_ptr<std::vector<char> > buffer = little_cache<std::vector<char> >::get(); // new: cache empty
 *         buffer->resize(1000);
 *     }   // the vector goes back into the cache
 *     {
 *         std::shared_ptr<std::vector<char> > buffer = little_cache<std::vector<char> >::get(); // the same vector,
 *     }                                                                                         // still 1000 long
 *
 * @tparam TYPE  - the type of objects to be cached
 * @tparam ID    - optional cache identifier, so you can have multiple independent caches of the same type
 * @tparam RESET - optional reset policy, applied to objects as they are returned (see NOTE5)
 */
template<typename TYPE, int ID = 0, typename RESET = little_cache_keep>
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
    little_cache& operator=(const little_cache&) = delete;

    /**
     * Get an object from the cache if there is one available, or construct one if not.  It goes back into the cache
     * when the last copy of the returned shared_ptr is destroyed.
     *
     * Objects returned may be brand new, or may be used.  It is your responsibility to set the object up when you get
     * it, and to remove anything private before it is recycled (or to use a reset policy that does).
     *
     * @return std::shared_ptr<TYPE> - an item for you to use - it may be new or used
     * @throws whatever TYPE's default constructor throws, or std::bad_alloc
     */
    static std::shared_ptr<TYPE> get(){
        //try to get a used item
        TYPE* t{little_cache<TYPE,ID,RESET>::tryGetFromCache()};
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
                returned = little_cache<TYPE,ID,RESET>::tryReturn(used);
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
        little_cache<TYPE,ID,RESET>* me = little_cache<TYPE,ID,RESET>::getCache();
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
        little_cache<TYPE,ID,RESET>* me = little_cache<TYPE,ID,RESET>::getCache();
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
     * @return a pointer to the singleton little_cache<TYPE,ID,RESET>
     */
    static little_cache<TYPE,ID,RESET>* getCache(){
        static little_cache<TYPE,ID,RESET>* const instance{new little_cache<TYPE,ID,RESET>()};
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
        little_cache<TYPE,ID,RESET>* me = little_cache::getCache();
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
        little_cache<TYPE,ID,RESET>* me = little_cache::getCache();
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
