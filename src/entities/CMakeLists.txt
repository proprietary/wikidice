add_library(
    entities 
    STATIC
)
target_sources(
    entities

    PUBLIC
    entities.h

    PRIVATE
    common.h
    msgpack_serde_logic.h
    entities.cc
)
target_compile_features(
    entities
    PRIVATE 
    cxx_std_23
)
target_compile_options(
    entities
    PRIVATE
    -Wall
    -Wextra
    -Werror
)
target_link_libraries(
    entities
    PRIVATE
    msgpack-cxx
    absl::check
)


# Testing

include(CTest)
enable_testing()
include(GoogleTest)


add_executable(
    entities_test
    entities_test.cc
)
target_link_libraries(
    entities_test
    PRIVATE
    entities
    gtest_main
)
target_compile_features(
    entities_test
    PRIVATE 
    cxx_std_23
)
target_link_options(
    entities_test
    PRIVATE
    -fsanitize=undefined,address
)
target_compile_options(
    entities_test
    PRIVATE
    -Wall
    -Wextra
    -Werror
    -fsanitize=undefined,address
    -fno-omit-frame-pointer
)

gtest_discover_tests(entities_test)