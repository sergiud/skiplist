find_package (Boost 1.47 REQUIRED COMPONENTS unit_test_framework)

if (NOT Boost_USE_STATIC_LIBS)
  add_definitions (-DBOOST_TEST_DYN_LINK)
endif (NOT Boost_USE_STATIC_LIBS)

if (WIN32)
  find_package (VLD)
endif (WIN32)

find_package (skiplist REQUIRED)

file (GLOB PROJECT_HPP ${skiplist_INCLUDE_DIR}/*.hpp)

include_directories (${Boost_INCLUDE_DIRS})
include_directories (${skiplist_INCLUDE_DIRS} ${skiplist_INCLUDE_DIR})

if (VLD_FOUND)
  add_definitions (-DHAVE_VLD)

  include_directories (${VLD_INCLUDE_DIRS})
  link_directories (${VLD_LIBRARY_DIRS})
endif (VLD_FOUND)

if (WIN32)
  add_definitions (-DNOMINMAX)
endif (WIN32)

if (MSVC)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif (MSVC)

add_definitions (${skiplist_DEFINITIONS})

add_executable (test_skiplist unittest.cpp ${PROJECT_HPP})
add_executable (test_benchmark benchmark.cpp ${PROJECT_HPP})

target_link_libraries (test_skiplist ${Boost_LIBRARIES} ${skiplist_LIBRARIES}
  ${skiplist_LIBRARY})

add_test (NAME skiplist-sorting COMMAND test_skiplist -t sorting)
add_test (NAME skiplist-insertion COMMAND test_skiplist -t insertion)
add_test (NAME skiplist-deletion COMMAND test_skiplist -t deletion)
add_test (NAME skiplist-duplicates COMMAND test_skiplist -t duplicates)
add_test (NAME skiplist-iterators COMMAND test_skiplist -t iterators)
add_test (NAME skiplist-copy COMMAND test_skiplist -t copy)
add_test (NAME skiplist-swap COMMAND test_skiplist -t swap)
add_test (NAME skiplist-comparison COMMAND test_skiplist -t comparison)
add_test (NAME skiplist-find COMMAND test_skiplist -t find)
add_test (NAME skiplist-map COMMAND test_skiplist -t map)
add_test (NAME skiplist-map-range COMMAND test_skiplist -t range)

if (HAVE_CXX0X) # set by skiplist config
  add_test (NAME skiplist-move COMMAND test_skiplist -t move)
endif (HAVE_CXX0X)
