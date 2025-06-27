/**********************************************************************
 * test_little_cache.cpp
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

#include <cassert>
#include <complex>
#include <iostream>
#include <memory>
#include <vector>
#include "../include/little_cache.h"

/**
 * So what I'm going to do here:
 * - get an item from the cache, and make sure it is empty
 * - add two items to it
 * - get a second item from the cache, and make sure it is also empty
 * - add one item to it
 * - delete it
 * - delete the first one
 *
 * - get an item from the cache, and make sure it has only one item (the most recently deleted item
 * - get a second item from the cache, and make sure it has two items (the second most recently deleted item
 * - get a third item from the cache, and make sure it is empty
 * @return
 */
int main(){
    std::cout << "Testing..." << std::endl;
    typedef std::vector<std::complex<float> > ITEM_TYPE;
    typedef std::shared_ptr<ITEM_TYPE> ITEM;

    {
        ITEM myItem1 = little_cache<ITEM_TYPE>::get();
        assert(0 == myItem1->size());
        std::cout << "first is new: " << (0 == myItem1->size()) << std::endl;
        myItem1->emplace_back(0.0,0.0);
        myItem1->emplace_back(0.0,0.0);

        {
            ITEM myItem2 = little_cache<ITEM_TYPE>::get();
            assert(0 == myItem2->size());
            std::cout << "second is new: " << (0 == myItem2->size()) << std::endl;
            myItem2->emplace_back(0.0,0.0);
        }
    }

    ITEM myItem1 = little_cache<ITEM_TYPE>::get();
    assert(2 == myItem1->size());
    std::cout << "got the correct used item: " << (2 == myItem1->size()) << std::endl;
    ITEM myItem2 = little_cache<ITEM_TYPE>::get();
    assert(1 == myItem2->size());
    std::cout << "got the correct used item: " << (1 == myItem2->size()) << std::endl;

    ITEM myItem3 = little_cache<ITEM_TYPE>::get();
    assert(myItem3->empty());
    std::cout << "got a new item: " << (myItem3->empty()) << std::endl;

    return 0;
}