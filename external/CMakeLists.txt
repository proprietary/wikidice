cmake_minimum_required(VERSION 3.14)

include(FetchContent)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(googletest-1.14.0)

# Abseil
set(ABSL_PROPAGATE_CXX_STD ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(abseil-cpp)

# fmt
add_subdirectory(fmt-10.1.0)

# Boost
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/boost_1_84_0)

# msgpack
FetchContent_Declare(
    msgpack-cxx
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/msgpack-cxx-6.1.0
)
FetchContent_MakeAvailable(msgpack-cxx)

# zstandard
set(ZSTD_BUILD_STATIC ON)
set(ZSTD_BUILD_SHARED OFF)
set(ZSTD_BUILD_TESTS OFF)
FetchContent_Declare(
    zstd
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zstd-1.5.5
    SOURCE_SUBDIR build/cmake
)
FetchContent_MakeAvailable(zstd)

# RocksDB
include(CompileRocksDB.cmake)

# Pybind11
add_subdirectory(pybind11-2.11.1)