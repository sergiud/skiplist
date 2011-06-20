find_package (skiplist REQUIRED)
find_package (Boost REQUIRED COMPONENTS unit_test_framework)

include_directories (${Boost_INCLUDE_DIRS})
include_directories (${skiplist_INCLUDE_DIRS} ${skiplist_INCLUDE_DIR})

add_executable (test_skiplist main.cpp)

target_link_libraries (test_skiplist ${Boost_LIBRARIES} ${skiplist_LIBRARIES}
  ${skiplist_LIBRARY})
