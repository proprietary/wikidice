include(ExternalProject)

set(RocksDB_CMAKE_ARGS
  -DUSE_RTTI=1
  -DFAIL_ON_WARNINGS=OFF
  -DWITH_GFLAGS=OFF
  -DWITH_TESTS=OFF
  -DWITH_TOOLS=OFF
  -DWITH_CORE_TOOLS=OFF
  -DWITH_BENCHMARK_TOOLS=OFF
  -DWITH_BZ2=OFF
  -DWITH_LZ4=ON
  -DWITH_SNAPPY=OFF
  -DWITH_ZLIB=OFF
  -DWITH_ZSTD=OFF
  -DROCKSDB_BUILD_SHARED=OFF
  -DCMAKE_POSITION_INDEPENDENT_CODE=True
  -DCMAKE_INSTALL_PREFIX="${CMAKE_BINARY_DIR}/rocksdb"
)
ExternalProject_Add(
  rocksdb
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/rocksdb-8.9.1"
  CMAKE_ARGS ${RocksDB_CMAKE_ARGS}
)

add_library(rocksdb-compiled INTERFACE)
add_dependencies(rocksdb-compiled rocksdb)
ExternalProject_Get_Property(rocksdb INSTALL_DIR)
target_link_libraries(rocksdb-compiled INTERFACE "${INSTALL_DIR}/lib/librocksdb.a")
target_include_directories(rocksdb-compiled INTERFACE "${INSTALL_DIR}/include")