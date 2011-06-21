set (skiplist_DEFINITIONS)
set (skiplist_INCLUDE_DIRS)
set (skiplist_LIBRARIES)
set (skiplist_LIBRARY)

include (CheckCXX0X)

check_has_cpp0x (HAVE_CPP0X)

if (HAVE_CPP0X)
  list (APPEND skiplist_DEFINITIONS -DHAVE_CPP0X)
endif (HAVE_CPP0X)

get_filename_component (_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component (skiplist_INCLUDE_DIR ${_DIR} ABSOLUTE)
