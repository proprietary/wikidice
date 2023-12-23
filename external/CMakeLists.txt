cmake_minimum_required(VERSION 3.14)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(googletest-1.14.0)
set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(abseil-cpp)
add_subdirectory(sqlite3)
add_subdirectory(fmt-10.1.0)
set(SPDLOG_FMT_EXTERNAL ON)
add_subdirectory(spdlog-1.12.0)
#add_subdirectory(glog-0.6.0)
#add_subdirectory(leveldb-1.23)
#add_subdirectory(mariadb-connector-c-3.3.8-src)
#add_subdirectory(mariadb-connector-cpp-1.0.2-src)
#add_subdirectory(rapidjson-1.1.0)

# Boost
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/boost_1_84_0)

# RocksDB
include(CompileRocksDB.cmake)
