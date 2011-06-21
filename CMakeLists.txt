cmake_minimum_required (VERSION 2.8)
project (Skiplist)

enable_testing ()

set (skiplist_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if (NOT UNIX OR APPLE)
  option (Boost_USE_STATIC_LIBS "Use static Boost libraries" ON)
endif (NOT UNIX OR APPLE)

set (CMAKE_COLOR_MAKEFILE ON)

add_subdirectory (tests)
