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

#include <memory>
#include <mutex>
#include <stack>
#include <type_traits>
#include <vector>


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
 *
 * example usage:
 * typedef std::vector<std::complex<float> > ITEM_TYPE;
 * typedef std::shared_ptr<ITEM_TYPE> ITEM;
 * //little_cache<ITEM_TYPE> is empty//
 * {
 *    ITEM myItem = little_cache<ITEM_TYPE>::get();
 *    //little_cache<ITEM_TYPE> is empty//
 * }
 * //little_cache<ITEM_TYPE> contains one item//
 * @tparam TYPE
 */
template<typename TYPE, int X = 0>
class little_cache{
public:
    static_assert(std::is_default_constructible<TYPE>::value);

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
     */
    static std::shared_ptr<TYPE> get(){
        //try to get a used item
        TYPE* t{little_cache<TYPE,X>::tryGetFromCache()};
        if(nullptr == t){
            //no used item available, make a new one
            t = new TYPE();
        }

        //make a deleter
        auto deleter = [](TYPE* used){
            if(!little_cache<TYPE,X>::tryReturn(used)){
                delete used;
                used = nullptr;
            }
        };

        //put the two together
        return std::shared_ptr<TYPE>(t, deleter);
    }


protected:
    std::stack<TYPE*> things;
    std::mutex thingsLock;

    little_cache(){};
    ~little_cache();

    /**
     * Get a singleton instance.
     * @return a pointer to the singleton little_cache<TYPE>
     */
    static little_cache<TYPE,X>* getCache(){
        static little_cache<TYPE,X>* ptr{nullptr};
        static std::mutex ptrLock;
        if(nullptr == ptr){
            std::lock_guard<std::mutex> lock(ptrLock);
            if(nullptr == ptr){
                ptr = new little_cache<TYPE,X>();
            }
        }
        return ptr;
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
        little_cache<TYPE,X>* me = little_cache::getCache();
        std::lock_guard<std::mutex> lock(me->thingsLock);
        me->things.push(t);
        return true;
    }

    /**
     * Like the name implies, try to get a used item.  If I don't think one is available, I'll
     * just return you a nullptr;
     * @return
     */
    static TYPE* tryGetFromCache(){
        little_cache<TYPE,X>* me = little_cache::getCache();
        if(me->things.empty()){
            //fail fast and lock-less
            return nullptr;
        }
        std::lock_guard<std::mutex> lock(me->thingsLock);
        if(me->things.empty()){
            //I must have been faked out, and another thread stole the last used item while we were
            //waiting for that lock.
            return nullptr;
        }

        //a used item exists - return it
        TYPE* t = me->things.top();
        me->things.pop();
        return t;
    }
};
