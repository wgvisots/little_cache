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
#use -fanalyzer in the future
set(CMAKE_CXX_FLAGS "-FPIC -fopenmp -Wall -Wextra -Wpedantic -g -pg -O2 --coverage")

enable_testing()

add_subdirectory(test)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    little_cache.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
    )

install(FILES ${CMAKE_SOURCE_DIR}/include/little_cache.h DESTINATION "include/little_cache")

set(CPACK_GENERATOR "DEB" "TGZ")
set(CPACK_PACKAGE_NAME "little_cache")
set(CPACK_PACKAGE_VENDOR "NMERC")
set(CPACK_PACKAGE_CONTACT "Greg Visotski")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Header-only simple cache library.")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGING_INSTALL_PREFIX /usr/)
set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}.${CMAKE_SYSTEM_PROCESSOR})
include(CPack)