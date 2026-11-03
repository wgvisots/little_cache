###########################################################################
# CMakeLists.txt
#
# Copyright 2025 W. Greg Visotski
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################

find_package(GTest   REQUIRED)
find_package(Threads REQUIRED)

add_executable(runUnitTests little_cache_test.cpp)

target_compile_options(runUnitTests PRIVATE -Wall -Wextra -Wpedantic -Werror -g)
target_link_libraries (runUnitTests PRIVATE little_cache::little_cache
                                            GTest::gtest
                                            GTest::gtest_main
                                            Threads::Threads)

if(LITTLE_CACHE_COVERAGE)
    target_compile_options(runUnitTests PRIVATE --coverage -O0)
    target_link_options   (runUnitTests PRIVATE --coverage)
endif()

if(LITTLE_CACHE_SANITIZER)
    target_compile_options(runUnitTests PRIVATE -fsanitize=${LITTLE_CACHE_SANITIZER} -fno-omit-frame-pointer)
    target_link_options   (runUnitTests PRIVATE -fsanitize=${LITTLE_CACHE_SANITIZER})
endif()

add_test(NAME runUnitTests COMMAND runUnitTests)
