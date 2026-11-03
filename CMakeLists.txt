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

project(little_cache
        VERSION      1.1.0
        DESCRIPTION  "Header-only simple cache library."
        HOMEPAGE_URL "https://github.com/wgvisots/little_cache"
        LANGUAGES    CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(LITTLE_CACHE_BUILD_TESTS "Build the unit tests"                                  ${PROJECT_IS_TOP_LEVEL})
option(LITTLE_CACHE_COVERAGE    "Instrument the tests for coverage"                    OFF)
set   (LITTLE_CACHE_SANITIZER   "" CACHE STRING "Sanitizers for the tests, e.g. address,undefined or thread")

################################
# The library: header only
################################
add_library(little_cache INTERFACE)
add_library(little_cache::little_cache ALIAS little_cache)

target_compile_features   (little_cache INTERFACE cxx_std_20)
target_include_directories(little_cache INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

################################
# Tests
################################
if(LITTLE_CACHE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

################################
# Install and package: find_package(little_cache) gives you little_cache::little_cache
################################
set(LITTLE_CACHE_CMAKE_DIR "${CMAKE_INSTALL_DATADIR}/cmake/little_cache")

install(TARGETS little_cache EXPORT little_cacheTargets)
install(DIRECTORY   include/little_cache DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT      little_cacheTargets
        NAMESPACE   little_cache::
        DESTINATION ${LITTLE_CACHE_CMAKE_DIR})

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/little_cacheConfigVersion.cmake"
                                 COMPATIBILITY SameMajorVersion
                                 ARCH_INDEPENDENT)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/little_cacheConfig.cmake"
     "include(\"\${CMAKE_CURRENT_LIST_DIR}/little_cacheTargets.cmake\")\n")
install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/little_cacheConfig.cmake"
                    "${CMAKE_CURRENT_BINARY_DIR}/little_cacheConfigVersion.cmake"
        DESTINATION ${LITTLE_CACHE_CMAKE_DIR})

set(CPACK_GENERATOR                   "DEB" "TGZ")
set(CPACK_PACKAGE_NAME                "little_cache")
set(CPACK_DEBIAN_PACKAGE_NAME         "little-cache") # Debian package names may only use a-z, 0-9, '+', '-' and '.'
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")          # header only
set(CPACK_DEBIAN_FILE_NAME            DEB-DEFAULT)
set(CPACK_PACKAGE_VENDOR              "NMERC")
set(CPACK_PACKAGE_CONTACT             "Greg Visotski")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VERSION             ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE       "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README        "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGING_INSTALL_PREFIX    /usr/)
set(CPACK_PACKAGE_FILE_NAME           ${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}.${CMAKE_SYSTEM_PROCESSOR})
include(CPack)
