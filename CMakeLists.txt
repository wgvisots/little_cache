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

cmake_minimum_required(VERSION 3.20)

set(PROJ thing)
project(${PROJ})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-FPIC -fopenmp -Wall -Wextra -Wpedantic -g -pg -O2")

set(LIBS pthread
         rt)

#file(GLOB_RECURSE SRCS src/*.cpp)
file(GLOB_RECURSE TEST_SRCS test/*.cpp)


#add_library(${PROJ} ${SRCS})
#target_include_directories(${PROJ} PUBLIC ${INC_DIRS})
#target_link_libraries(${PROJ} ${LIBS})

add_executable(${PROJ}_test ${TEST_SRCS})
target_include_directories(${PROJ}_test PUBLIC ${INC_DIRS})
target_link_libraries(${PROJ}_test ${LIBS})
