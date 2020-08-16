# Copyright (c) 2011, 2020 Sergiu Deitsch
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

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
