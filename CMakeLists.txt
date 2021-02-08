cmake_minimum_required(VERSION 3.17)
project(DFL)

set(CMAKE_CXX_STANDARD 17)

add_executable(DFL main.cpp)

# OS
if (WIN32)
    message(STATUS "Operation system is WIN32")
    set(BOOST_ROOT "C:\\lib\\boost_1_75_0")
elseif(APPLE)
    message(STATUS "Operation system is APPLE")
    set(BOOST_ROOT "C:\\lib\\boost_1_75_0")
elseif(UNIX)
    message(STATUS "Operation system is UNIX")
    set(BOOST_ROOT "C:\\lib\\boost_1_75_0")
endif ()


# Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.75.0 COMPONENTS FILESYSTEM)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(DFL ${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "Boost not found")
endif()
