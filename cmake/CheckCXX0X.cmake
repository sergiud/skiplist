MACRO (check_has_cpp0x VARIABLE)
  IF ("${VARIABLE}" MATCHES "^${VARIABLE}$")
    MESSAGE (STATUS "Checking C++0x support")

    IF (MSVC10)
      SET (${VARIABLE} 1) # Visual Studio 10 supports C++0x
    ELSE (MSVC10)
      TRY_COMPILE (${VARIABLE}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_MODULE_PATH}/CheckCXX0X.cpp
        COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
        CMAKE_FLAGS ${CMAKE_REQUIRED_FLAGS}
        OUTPUT_VARIABLE OUTPUT)
    ENDIF (MSVC10)

    IF (${VARIABLE})
      SET (${VARIABLE} 1 CACHE INTERNAL "Have C++0x support")
      MESSAGE (STATUS "Checking C++0x support - available")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Determining if C++0x support passed with the following output:\n"
        "${OUTPUT}\n\n")
    ELSE (${VARIABLE})
      MESSAGE (STATUS "Checking C++0x support - not available")
      SET (${VARIABLE} "" CACHE INTERNAL "Have C++0x")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Determining if C++0x support is available passed with the following output:\n"
        "${OUTPUT}\n\n")
    ENDIF (${VARIABLE})
  ENDIF ("${VARIABLE}" MATCHES "^${VARIABLE}$")
ENDMACRO (check_has_cpp0x)
