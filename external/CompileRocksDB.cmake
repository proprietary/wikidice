include(ExternalProject)

ExternalProject_Add(
  zstd
  PREFIX "${CMAKE_BINARY_DIR}/zstd"
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/zstd-1.5.5"
  SOURCE_SUBDIR build/cmake
  CMAKE_ARGS -DZSTD_BUILD_STATIC=ON -DZSTD_BUILD_SHARED=OFF -DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_TESTS=OFF -DZSTD_LEGACY_SUPPORT=OFF
  BUILD_BYPRODUCTS <BINARY_DIR>/lib/libzstd.a
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(zstd BINARY_DIR SOURCE_DIR)
set(zstd_LIBRARIES "${BINARY_DIR}/lib/libzstd.a")

set(RocksDB_CMAKE_ARGS
  -DUSE_RTTI=1
  -DFAIL_ON_WARNINGS=OFF
  -DWITH_GFLAGS=OFF
  -DWITH_TESTS=OFF
  -DWITH_TOOLS=OFF
  -DWITH_CORE_TOOLS=OFF
  -DWITH_BENCHMARK_TOOLS=OFF
  -DWITH_BZ2=OFF
  -DWITH_LZ4=OFF
  -DWITH_SNAPPY=OFF
  -DWITH_ZLIB=OFF
  -DWITH_ZSTD=ON
  -DROCKSDB_BUILD_SHARED=OFF
  -DCMAKE_POSITION_INDEPENDENT_CODE=True
  -DCMAKE_CXX_FLAGS="-DZSTD_STATIC_LINKING_ONLY"
)
ExternalProject_Add(
  rocksdb
  PREFIX "${CMAKE_BINARY_DIR}/rocksdb"
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/rocksdb-8.10.0"
  CMAKE_ARGS ${RocksDB_CMAKE_ARGS}
  BUILD_BYPRODUCTS <BINARY_DIR>/librocksdb.a
  INSTALL_COMMAND ""
)

add_library(rocksdb-compiled INTERFACE)
add_dependencies(rocksdb-compiled rocksdb)
ExternalProject_Get_Property(rocksdb BINARY_DIR)
target_link_libraries(rocksdb-compiled INTERFACE "${BINARY_DIR}/librocksdb.a" "${zstd_LIBRARIES}")
ExternalProject_Get_Property(rocksdb SOURCE_DIR)
target_include_directories(rocksdb-compiled INTERFACE "${SOURCE_DIR}/include")
