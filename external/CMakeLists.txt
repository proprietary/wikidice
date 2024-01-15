cmake_minimum_required(VERSION 3.24)

include(FetchContent)
include(ExternalProject)

FetchContent_Declare(
    googletest
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest-1.14.0
    CMAKE_ARGS -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_STANDARD_REQUIRED=ON
)
FetchContent_MakeAvailable(googletest)

# Abseil
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_USE_GOOGLETEST_HEAD OFF CACHE BOOL "" FORCE)
set(ABSL_CXX_STANDARD 20 CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 20)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    abseil-cpp
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/abseil-cpp
)
FetchContent_MakeAvailable(abseil-cpp)

# fmt
add_subdirectory(fmt-10.1.0)

# Boost
set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/boost_1_84_0")
find_package(Boost REQUIRED)
add_library(boost_headers INTERFACE)
target_include_directories(boost_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/boost_1_84_0)


# msgpack
FetchContent_Declare(
    msgpack-cxx
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/msgpack-cxx-6.1.0
)
FetchContent_MakeAvailable(msgpack-cxx)

# RocksDB
include(CompileRocksDB.cmake)

# Pybind11
add_subdirectory(pybind11-2.11.1)
