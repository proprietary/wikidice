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
)
ExternalProject_Add(
    rocksdb-compile
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/rocksdb-8.9.1"
    CMAKE_ARGS ${RocksDB_CMAKE_ARGS}
)
ExternalProject_Get_Property(rocksdb-compile BINARY_DIR)
add_library(rocksdb STATIC IMPORTED)
set_target_properties(rocksdb PROPERTIES IMPORTED_LOCATION ${BINARY_DIR}/librocksdb.a)
add_dependencies(rocksdb rocksdb-compile)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/external/rocksdb/lib/cmake/rocksdb)
