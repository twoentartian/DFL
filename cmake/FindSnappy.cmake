# This file was taken from the Caffe project: https://github.com/lemire/FastPFor
# It has been adapted to also find the snappy windows binaries.

# This code is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
# Copyright (c) 2012 Louis Dionne
#
# Find snappy compression library and includes. This module defines:
#   snappy_INCLUDE_DIRS - The directories containing snappy's headers.
#   snappy_LIBRARIES    - A list of snappy's libraries.
#   snappy_DLLS         - A list of snappy's DLL on windows.
#   snappy_FOUND        - Whether snappy was found.
#
# This module can be controlled by setting the following variables:
#   snappy_ROOT - The root directory where to find snappy. If this is not
#                 set, the default paths are searched.

if(NOT snappy_ROOT AND (NOT "$ENV{SNAPPY_ROOT}" EQUAL ""))
    set(snappy_ROOT $ENV{SNAPPY_ROOT})
    message(STATUS "Read snappy root-directory from environment variable SNAPPY_ROOT: ${snappy_ROOT}")
endif()

if(NOT snappy_ROOT)
    find_path(snappy_INCLUDE_DIRS snappy.h)
    find_library(snappy_LIBRARIES NAMES snappy)
else()
    find_path(snappy_INCLUDE_DIRS snappy.h NO_DEFAULT_PATH PATHS ${snappy_ROOT} ${snappy_ROOT}/include)
    find_library(snappy_LIBRARIES NAMES snappy NO_DEFAULT_PATH PATHS ${snappy_ROOT} ${snappy_ROOT}/lib)
endif()

if(snappy_INCLUDE_DIRS AND NOT snappy_LIBRARIES)
    if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
        find_library(snappy_LIBRARIES NAMES snappy64 NO_DEFAULT_PATH PATHS ${snappy_ROOT} ${snappy_ROOT}/native)
        find_file(snappy_DLLS NAMES "snappy64.dll" NO_DEFAULT_PATH PATHS ${snappy_ROOT}/native)
    else()
        find_library(snappy_LIBRARIES NAMES snappy32 NO_DEFAULT_PATH PATHS ${snappy_ROOT} ${snappy_ROOT}/native)
        find_file(snappy_DLLS NAMES "snappy32.dll" NO_DEFAULT_PATH PATHS ${snappy_ROOT}/native)
    endif()
endif()

if(snappy_INCLUDE_DIRS AND snappy_LIBRARIES)
    set(snappy_FOUND TRUE)
    message(STATUS "Found Snappy:")
    message(STATUS " * Snappy include path: ${snappy_INCLUDE_DIRS}")
    message(STATUS " * Snappy library:      ${snappy_LIBRARIES}")
    if (snappy_DLLS)
        message(STATUS " * Snappy DLL:      ${snappy_DLLS}")
    endif()
else()
    set(snappy_FOUND FALSE)
    set(snappy_INCLUDE_DIR)
    set(snappy_LIBRARIES)
    set(snappy_DLLS)
endif()

mark_as_advanced(snappy_LIBRARIES snappy_INCLUDE_DIRS snappy_DLLS)