add_library(
    file_utils
    STATIC
)
target_sources(
    file_utils
    PUBLIC
    file_utils.h
    PRIVATE
    file_utils.cc
)
target_compile_features(
    file_utils
    PRIVATE
    cxx_std_23
)
target_compile_options(
    file_utils
    PRIVATE
    -Wall -Wextra -Werror
)


include(CTest)
enable_testing()
include(GoogleTest)

add_executable(
    file_utils_test
    file_utils_test.cc
)
target_link_libraries(
    file_utils_test
    PRIVATE
    file_utils
    gtest_main
)
target_compile_features(
    file_utils_test
    PRIVATE
    cxx_std_23
)
target_compile_options(
    file_utils_test
    PRIVATE
    -Wall -Wextra -Werror
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
)
target_link_options(
    file_utils_test
    PRIVATE
    -fsanitize=address,undefined
)
gtest_discover_tests(file_utils_test)