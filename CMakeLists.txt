cmake_minimum_required (VERSION 2.8)
project (Skiplist)

set (skiplist_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory (tests)
