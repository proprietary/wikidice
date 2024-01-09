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
find_package(Python 3.11 COMPONENTS Interpreter Development REQUIRED)

# External dependencies
add_subdirectory(external)

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

add_library(primitive_serializer STATIC)
target_sources(
    primitive_serializer
    PRIVATE src/cross_platform_endian.h src/primitive_serializer.h src/primitive_serializer.cc
    PUBLIC src/primitive_serializer.h
)
target_link_libraries(primitive_serializer PUBLIC absl::span)
target_compile_features(primitive_serializer PRIVATE cxx_std_23)
target_compile_options(primitive_serializer PRIVATE -Wall -Wextra -pedantic -Werror)
target_link_options(primitive_serializer PRIVATE -fuse-ld=lld)

add_library(libwikidice STATIC)
target_sources(
    libwikidice
    PUBLIC
    src/cross_platform_set_thread_name.h
    src/category_link_type.h src/category_link_type.cc
    src/category_table.h src/category_table.cc
    src/sql_parser.h src/sql_parser.cc
    src/bounded_string_ring.h src/bounded_string_ring.cc
    src/category_tree_index.h src/category_tree_index.cc
    src/wikidice_query.h src/wikidice_query.cc
    src/category_link_type.h
    src/sql_parser.h
    src/category_tree_index.h
    src/wikidice_query.h
)
target_compile_features(
    libwikidice
    PRIVATE
    cxx_std_23
)
# position independent code
set_target_properties(
    libwikidice
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
target_compile_options(
    libwikidice
    PRIVATE
    -Wno-gcc-compat
    -Wall -Wextra -pedantic -Werror -g3
    #-fsanitize=undefined,address -fno-omit-frame-pointer
)
target_link_options(
    libwikidice
    PRIVATE
    -fuse-ld=lld
    #-fsanitize=undefined,address
)
target_link_libraries(
    libwikidice
    PUBLIC
    primitive_serializer
    fmt::fmt Threads::Threads ${Boost_LIBRARIES}
    absl::flags absl::flags_usage absl::flags_parse
    absl::strings absl::str_format absl::time absl::base absl::hash absl::flat_hash_map
    absl::log absl::absl_check absl::log_flags absl::log_sink
    absl::random_random absl::random_bit_gen_ref
    absl::btree absl::flat_hash_map absl::flat_hash_set absl::node_hash_map absl::node_hash_set
    absl::span
    rocksdb-compiled
    msgpack-cxx
    libzstd_static
)
target_include_directories(
    libwikidice
    PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/external/boost_1_84_0"
    #"${CMAKE_CURRENT_LIST_DIR}/external/abseil-cpp"
    "${CMAKE_CURRENT_LIST_DIR}/external/zstd-1.5.5/lib"
)

add_executable(wikidice_builder)
target_sources(
    wikidice_builder
    PUBLIC
    src/build_category_tree.h
    src/build_category_tree.cc
)
target_compile_features(wikidice_builder PRIVATE cxx_std_23)
target_compile_options(
    wikidice_builder
    PRIVATE
    -Wno-gcc-compat
    -Wall -Wextra -pedantic -Werror -g3
    #-fsanitize=undefined,address -fno-omit-frame-pointer
)
target_link_options(
    wikidice_builder
    PRIVATE
    -fuse-ld=lld
    #-fsanitize=undefined,address
)
target_link_libraries(
    wikidice_builder
    PRIVATE
    libwikidice fmt::fmt Threads::Threads ${Boost_LIBRARIES}
)
target_include_directories(wikidice_builder PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/boost_1_84_0)

# Python module
pybind11_add_module(pywikidice src/pywikidice.cc)
target_compile_features(pywikidice PRIVATE cxx_std_23)
#target_compile_options(pywikidice PRIVATE -Wno-gcc-compat -Wall -Wextra -pedantic -Werror -g3 -fsanitize=address,undefined -fno-omit-frame-pointer)
#target_link_options(pywikidice PRIVATE -fuse-ld=lld -fsanitize=address,undefined)
target_link_libraries(pywikidice PRIVATE libwikidice)


# Setup our project as the startup project for Visual so that people don't need to do it manually
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT WikiDice)


#===========#
#   Tests   #
#===========#

include(CTest)
enable_testing()

add_executable(sql_parser_test src/sql_parser_test.cc)
target_compile_features(sql_parser_test PRIVATE cxx_std_23)
target_compile_options(
    sql_parser_test PRIVATE -Wno-gcc-compat -Wall -Wextra -pedantic -Werror -g3
    #-fsanitize=undefined,address -fno-omit-frame-pointer
)
target_link_libraries(sql_parser_test PRIVATE gtest_main gmock_main gmock libwikidice)
target_link_options(sql_parser_test PRIVATE
    -fsanitize=undefined,address
    -fuse-ld=lld
)

add_executable(category_tree_index_test src/category_tree_index_test.cc)
target_compile_features(category_tree_index_test PRIVATE cxx_std_23)
target_compile_options(category_tree_index_test PRIVATE -Wno-gcc-compat -Wall -Wextra -pedantic -Werror -g3 -fsanitize=undefined,address -fno-omit-frame-pointer)
target_link_libraries(category_tree_index_test PRIVATE gtest_main gmock_main gmock libwikidice)
target_link_options(category_tree_index_test PRIVATE -fsanitize=undefined,address -fuse-ld=lld)

add_executable(primitive_serializer_test src/primitive_serializer_test.cc)
target_compile_features(primitive_serializer_test PRIVATE cxx_std_23)
target_compile_options(primitive_serializer_test PRIVATE -Wno-gcc-compat -Wall -Wextra -pedantic -Werror -g3 -fsanitize=undefined,address -fno-omit-frame-pointer)
target_link_libraries(primitive_serializer_test PRIVATE primitive_serializer gtest_main gmock_main gmock absl::span)
target_link_options(primitive_serializer_test PRIVATE -fsanitize=undefined,address -fuse-ld=lld)

include(GoogleTest)
gtest_discover_tests(sql_parser_test)
gtest_discover_tests(category_tree_index_test)
