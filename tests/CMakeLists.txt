find_package (skiplist REQUIRED)
find_package (Boost REQUIRED COMPONENTS unit_test_framework)
find_package (VLD)

include_directories (${Boost_INCLUDE_DIRS})
include_directories (${skiplist_INCLUDE_DIRS} ${skiplist_INCLUDE_DIR})
include_directories (${VLD_INCLUDE_DIR})

if (VLD_FOUND)
  add_definitions (-DHAVE_VLD)
endif (VLD_FOUND)

if (WIN32)
  add_definitions (-DNOMINMAX)
endif (WIN32)

if (MSVC)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif (MSVC)

add_executable (test_skiplist main.cpp)

target_link_libraries (test_skiplist ${Boost_LIBRARIES} ${skiplist_LIBRARIES}
  ${skiplist_LIBRARY})
