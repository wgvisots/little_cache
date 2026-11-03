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
#include <atomic>
#include <algorithm>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"

#include "../include/little_cache.h"

namespace {
using ITEM_TYPE = std::vector<std::complex<float> >;
using ITEM = std::shared_ptr<ITEM_TYPE>;
}

TEST(LittleCache, TestConstruct) {
    little_cache<ITEM_TYPE>::purge();
    {
        ITEM myItem1 = little_cache<ITEM_TYPE>::get();
        EXPECT_NE(nullptr, myItem1.get());
        EXPECT_EQ(0, myItem1->size());
    }
    little_cache<ITEM_TYPE>::purge();
}

TEST(LittleCache, TestMakesNewThings) {
    little_cache<ITEM_TYPE>::purge();
    {
        ITEM myItem1 = little_cache<ITEM_TYPE>::get();
        EXPECT_NE(nullptr, myItem1.get());
        EXPECT_EQ(0, myItem1->size());
        myItem1->emplace_back(0.0, 0.0);
        myItem1->emplace_back(0.0, 0.0);

        ITEM myItem2 = little_cache<ITEM_TYPE>::get();
        EXPECT_NE(nullptr, myItem2.get());
        EXPECT_EQ(0, myItem2->size());
    }
    little_cache<ITEM_TYPE>::purge();
}

TEST(LittleCache, TestThingsGoToRest) {
    little_cache<ITEM_TYPE>::purge();
    {
        ITEM myItem1 = little_cache<ITEM_TYPE>::get();
        ITEM myItem2 = little_cache<ITEM_TYPE>::get();
        ITEM myItem3 = little_cache<ITEM_TYPE>::get();
    }
    ASSERT_EQ(3, little_cache<ITEM_TYPE>::numCachedItems());
    little_cache<ITEM_TYPE>::purge();
}

TEST(LittleCache, TestMultipleCaches) {
    little_cache<ITEM_TYPE,0>::purge();
    little_cache<ITEM_TYPE,1>::purge();
    {
        ITEM myItem0_1 = little_cache<ITEM_TYPE, 0>::get();
        ITEM myItem0_2 = little_cache<ITEM_TYPE, 0>::get();
        ITEM myItem0_3 = little_cache<ITEM_TYPE, 0>::get();
        myItem0_1->emplace_back(0.0, 0.0);
        myItem0_2->emplace_back(0.0, 0.0);
        myItem0_3->emplace_back(0.0, 0.0);

        ITEM myItem1_1 = little_cache<ITEM_TYPE, 1>::get();
        ITEM myItem1_2 = little_cache<ITEM_TYPE, 1>::get();
        myItem1_1->emplace_back(0.0, 0.0);
        myItem1_1->emplace_back(0.0, 0.0);
        myItem1_2->emplace_back(0.0, 0.0);
        myItem1_2->emplace_back(0.0, 0.0);
    }
    const std::size_t num0 = little_cache<ITEM_TYPE, 0>::numCachedItems();
    const std::size_t num1 = little_cache<ITEM_TYPE, 1>::numCachedItems();
    ASSERT_EQ(3, num0);
    ASSERT_EQ(2, num1);
    little_cache<ITEM_TYPE,0>::purge();
    little_cache<ITEM_TYPE,1>::purge();
}

TEST(LittleCache, TestThingsComeBack) {
    little_cache<ITEM_TYPE>::purge();
    //Make two things, put stuff in them, and then allow them to be collected.
    {
        ITEM myItem1 = little_cache<ITEM_TYPE>::get();
        myItem1->emplace_back(0.0, 0.0);
        myItem1->emplace_back(0.0, 0.0);
        ITEM myItem2 = little_cache<ITEM_TYPE>::get();
        myItem2->emplace_back(0.0, 0.0);
    }

    //now test and see if we get those thigns back
    {
        ITEM myItem1 = little_cache<ITEM_TYPE>::get();
        ASSERT_EQ(2, myItem1->size());
        ITEM myItem2 = little_cache<ITEM_TYPE>::get();
        ASSERT_EQ(1, myItem2->size());

        ITEM myItem3 = little_cache<ITEM_TYPE>::get();
        ASSERT_TRUE(myItem3->empty());
        myItem3->emplace_back(0.0, 0.0);
    }
    little_cache<ITEM_TYPE>::purge();
}

namespace {
/// Counts how many instances exist, so tests can tell whether items were really destroyed.
struct Counted {
    static inline std::atomic<int> alive{0};
    Counted() { alive++; }
    ~Counted() { alive--; }
    Counted(const Counted&)            = delete;
    Counted& operator=(const Counted&) = delete;
};
}

TEST(LittleCache, TestPurgeDestroysCachedItems) {
    little_cache<Counted>::purge();
    const int before = Counted::alive;
    {
        std::shared_ptr<Counted> a = little_cache<Counted>::get();
        std::shared_ptr<Counted> b = little_cache<Counted>::get();
    }
    ASSERT_EQ(2U, little_cache<Counted>::numCachedItems());
    ASSERT_EQ(before + 2, Counted::alive);

    little_cache<Counted>::purge();
    EXPECT_EQ(0U, little_cache<Counted>::numCachedItems());
    EXPECT_EQ(before, Counted::alive);
}

TEST(LittleCache, TestCacheSizeIsLimited) {
    little_cache<Counted, 1>::purge();
    const int         before = Counted::alive;
    const std::size_t limit  = little_cache<Counted, 1>::MAX_CACHED_ITEMS;
    const std::size_t extra{10};
    {
        std::vector<std::shared_ptr<Counted> > items;
        for (std::size_t i = 0; i < limit + extra; i++) {
            items.push_back(little_cache<Counted, 1>::get());
        }
    }
    // The cache keeps as many as it may; the rest are deleted.
    const std::size_t cached = little_cache<Counted, 1>::numCachedItems();
    EXPECT_EQ(limit, cached);
    EXPECT_EQ(before + static_cast<int>(limit), Counted::alive);

    little_cache<Counted, 1>::purge();
    EXPECT_EQ(before, Counted::alive);
}

TEST(LittleCache, TestManyThreadsAtOnce) {
    little_cache<Counted, 2>::purge();
    const int before = Counted::alive;
    {
        std::vector<std::thread> threads;
        for (int t = 0; t < 8; t++) {
            threads.emplace_back([] {
                for (int i = 0; i < 20000; i++) {
                    std::shared_ptr<Counted> first  = little_cache<Counted, 2>::get();
                    std::shared_ptr<Counted> second = little_cache<Counted, 2>::get();
                    EXPECT_NE(first.get(), second.get());
                }
            });
        }
        for (std::thread& thread : threads) {
            thread.join();
        }
    }
    // Every item is back at rest: nothing leaked, nothing deleted twice, nothing handed out twice.
    const std::size_t cached = little_cache<Counted, 2>::numCachedItems();
    EXPECT_LE(cached, 16U);
    EXPECT_EQ(before + static_cast<int>(cached), Counted::alive);

    little_cache<Counted, 2>::purge();
    EXPECT_EQ(before, Counted::alive);
}

namespace {
/// A custom reset policy: zero every element, but keep the size.
struct ZeroFill {
    void operator()(std::vector<int>& item) const noexcept {
        std::fill(item.begin(), item.end(), 0);
    }
};

/// Counts how often it is called.
struct CountResets {
    static inline std::atomic<int> calls{0};
    void operator()(Counted& /*item*/) const noexcept {
        calls++;
    }
};
}

TEST(LittleCache, TestKeepIsTheDefaultPolicy) {
    EXPECT_TRUE((std::is_same_v<little_cache<ITEM_TYPE>, little_cache<ITEM_TYPE, 0, little_cache_keep> >));
}

TEST(LittleCache, TestClearPolicyEmptiesReturnedItems) {
    little_cache<std::vector<int>, 0, little_cache_clear>::purge();
    {
        std::shared_ptr<std::vector<int> > item = little_cache<std::vector<int>, 0, little_cache_clear>::get();
        item->reserve(100);
        item->assign({1, 2, 3});
    }
    {
        // The same object comes back empty, but still has its memory.
        std::shared_ptr<std::vector<int> > item = little_cache<std::vector<int>, 0, little_cache_clear>::get();
        EXPECT_TRUE(item->empty());
        EXPECT_GE(item->capacity(), 100U);
    }
    little_cache<std::vector<int>, 0, little_cache_clear>::purge();
}

TEST(LittleCache, TestCustomPolicy) {
    little_cache<std::vector<int>, 0, ZeroFill>::purge();
    {
        std::shared_ptr<std::vector<int> > item = little_cache<std::vector<int>, 0, ZeroFill>::get();
        item->assign({1, 2, 3});
    }
    {
        std::shared_ptr<std::vector<int> > item = little_cache<std::vector<int>, 0, ZeroFill>::get();
        EXPECT_EQ((std::vector<int>{0, 0, 0}), *item);
    }
    little_cache<std::vector<int>, 0, ZeroFill>::purge();
}

TEST(LittleCache, TestPolicyRunsOnEveryReturnEvenWhenFull) {
    little_cache<Counted, 0, CountResets>::purge();
    const int         before = CountResets::calls;
    const std::size_t limit  = little_cache<Counted, 0, CountResets>::MAX_CACHED_ITEMS;
    {
        std::vector<std::shared_ptr<Counted> > items;
        for (std::size_t i = 0; i < limit + 5; i++) {
            items.push_back(little_cache<Counted, 0, CountResets>::get());
        }
    }
    // Reset before being cached, and before being deleted because the cache was full.
    EXPECT_EQ(before + static_cast<int>(limit) + 5, CountResets::calls);
    little_cache<Counted, 0, CountResets>::purge();
}

TEST(LittleCache, TestPoliciesAreSeparateCaches) {
    little_cache<std::vector<int>, 0, little_cache_keep>::purge();
    little_cache<std::vector<int>, 0, little_cache_clear>::purge();
    {
        std::shared_ptr<std::vector<int> > item = little_cache<std::vector<int>, 0, little_cache_clear>::get();
    }
    const std::size_t kept    = little_cache<std::vector<int>, 0, little_cache_keep>::numCachedItems();
    const std::size_t cleared = little_cache<std::vector<int>, 0, little_cache_clear>::numCachedItems();
    EXPECT_EQ(0U, kept);
    EXPECT_EQ(1U, cleared);
    little_cache<std::vector<int>, 0, little_cache_clear>::purge();
}
