find_package (Boost 1.47 REQUIRED COMPONENTS unit_test_framework)
find_package (VLD)
find_package (skiplist REQUIRED)

include_directories (${Boost_INCLUDE_DIRS})
include_directories (${skiplist_INCLUDE_DIRS} ${skiplist_INCLUDE_DIR})

if (VLD_FOUND)
  add_definitions (-DHAVE_VLD)

  include_directories (${VLD_INCLUDE_DIR})
  link_directories (${VLD_LIBRARIES_DIR})
endif (VLD_FOUND)

if (WIN32)
  add_definitions (-DNOMINMAX)
endif (WIN32)

if (MSVC)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif (MSVC)

add_definitions (${skiplist_DEFINITIONS})

add_executable (test_skiplist unittest.cpp)
add_executable (test_benchmark benchmark.cpp)

target_link_libraries (test_skiplist ${Boost_LIBRARIES} ${skiplist_LIBRARIES}
  ${skiplist_LIBRARY})

add_test (sorting test_skiplist -t sorting)
add_test (insertion test_skiplist -t deletion)
add_test (duplicates test_skiplist -t duplicates)
add_test (iterators test_skiplist -t iterators)
add_test (swap test_skiplist -t copy)
add_test (comparison test_skiplist -t comparison)

if (HAVE_CPP0X) # set by skiplist config
  add_test (move test_skiplist -t move)
endif (HAVE_CPP0X)
