project(sqlite3)

find_package(Threads REQUIRED)

add_library(sqlite3)
target_link_libraries(sqlite3 PRIVATE Threads::Threads)
target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=1)
target_sources(
    sqlite3
    PRIVATE sqlite3.c sqlite3.h sqlite3ext.h
    PUBLIC sqlite3.h sqlite3ext.h
)
add_library(sqlite3::sqlite3 ALIAS sqlite3)
