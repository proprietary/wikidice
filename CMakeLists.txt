cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

# Put the project early since modules might need to detect the compiler.
# More information https://cmake.org/cmake/help/latest/command/project.html
project(
    "WikiDice" # This will exposed as the variable PROJECT_NAME.
    VERSION 0.1.0 # Used for installation and defines variables PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH, and PROJECT_VERSION_TWEAK.
    LANGUAGES C CXX # Used to determine the languages to use based on file extensions
)

############################
##  Modules and scripts   ##
############################

# Standard CMake modules

include(CTest)                          # Must be called before adding tests but after calling project(). This automatically calls enable_testing() and configures ctest targets when using Make/Ninja
include(CMakeDependentOption)           # This is a really useful scripts that creates options that depends on other options. It can even be used with generator expressions !
include(GNUInstallDirs)                 # This will define the default values for installation directories (all platforms even if named GNU)
include(InstallRequiredSystemLibraries) # Tell CMake that the `install` target needs to install required system libraries (eg: Windows SDK)
include(CMakePackageConfigHelpers)      # Helper to create relocatable packages

# Custom modules and scripts

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake") # Make our cmake scripts available

include(LTO)
include(Warnings)
include(CopyDllsForDebug)
include(Coverage)

###############
##  OPTIONS  ##
###############

find_package(Threads REQUIRED)

# External dependencies
add_subdirectory(external)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/external/rocksdb/lib/cmake/rocksdb)
find_package(Boost COMPONENTS system program_options REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(RocksDB REQUIRED)

# It is always easier to navigate in an IDE when projects are organized in folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Whe building a shared library, you do not want to export all symbols by default
# gcc (and hence clang) are wrong about this.
#
# For more information, see https://gcc.gnu.org/wiki/Visibility and https://www.youtube.com/embed/m0DwB4OvDXk
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

###############
##  Project  ##
###############



# Check for LTO support (needs to be after project(...) )
find_lto(CXX)

# static analyzers

#find_program(CPPCHECK cppcheck REQUIRED)
#set(CMAKE_CXX_CPPCHECK ${CPPCHECK} --enable=all)

#find_program(CLANGTIDY clang-tidy REQUIRED)
#set(CMAKE_CXX_CLANG_TIDY ${CLANGTIDY})

# project

add_library(libwikidice STATIC)
target_sources(
    libwikidice
    PUBLIC
    src/category_link_type.h src/category_link_type.cc
    src/sql_parser.h src/sql_parser.cc
    src/bounded_string_ring.h src/bounded_string_ring.cc
    src/category_tree_index.h src/category_tree_index.cc
)
target_compile_features(libwikidice PRIVATE cxx_std_20)
target_compile_options(
    libwikidice
    PRIVATE
    -Wall -Wextra -pedantic -Werror -Wno-gcc-compat
    -fsanitize=undefined,address,leak -fno-omit-frame-pointer
)
target_link_options(
    libwikidice
    PRIVATE -fuse-ld=lld -fsanitize=undefined,address,leak
)
add_dependencies(libwikidice rocksdb-compile)
target_link_libraries(
    libwikidice
    PUBLIC
    fmt::fmt sqlite3::sqlite3 absl::time spdlog::spdlog_header_only Threads::Threads ${Boost_LIBRARIES}
    RocksDB::rocksdb
    absl::log absl::flat_hash_map
)
target_include_directories(
    libwikidice
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/external/boost_1_84_0
    "/Users/zds/src/zelcon.net/wikidice/_build/external/rocksdb/include"
    ${CMAKE_CURRENT_LIST_DIR}/external/com_github_nlohmann_json_v3.11.3/include
    ${CMAKE_CURRENT_LIST_DIR}/external/rapidjson-1.1.0/include
)

add_executable(WikiDice)
target_sources(
    WikiDice
    PRIVATE
    src/build_category_tree.cc
)
target_compile_features(WikiDice PRIVATE cxx_std_20)
target_compile_options(
    WikiDice
    PRIVATE
    -Wall -Wextra -pedantic -Werror -Wno-gcc-compat
    -fsanitize=undefined,address,leak -fno-omit-frame-pointer
)
target_link_options(
    WikiDice
    PRIVATE -fuse-ld=lld -fsanitize=undefined,address,leak
)
target_link_libraries(WikiDice PRIVATE libwikidice fmt::fmt sqlite3::sqlite3 absl::time spdlog::spdlog_header_only Threads::Threads ${Boost_LIBRARIES})
target_include_directories(WikiDice PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/boost_1_84_0)

# CMake scripts extensions
target_enable_lto(WikiDice optimized) #enable lto if available for non-debug configurations
copy_dlls_for_debug(WikiDice "" "") # Copy dependencies next to the executable (DLLs for example)

# Setup our project as the startup project for Visual so that people don't need to do it manually
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT WikiDice)


#===========#
#   Tests   #
#===========#

include(CTest)
enable_testing()

add_executable(sql_parser_test src/sql_parser_test.cc)
target_compile_features(sql_parser_test PRIVATE cxx_std_20)
target_link_libraries(sql_parser_test PRIVATE gtest_main gmock_main gmock libwikidice)

include(GoogleTest)
gtest_discover_tests(sql_parser_test)
