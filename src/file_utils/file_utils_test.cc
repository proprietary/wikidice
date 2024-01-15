#include "file_utils.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace net_zelcon::wikidice::file_utils;

class FilePortionStreamFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a temporary file
        temp_filename =
            std::filesystem::temp_directory_path() / "temp_file.txt";
        temp_file.open(temp_filename);
        temp_file << "Hello, World!";
        temp_file.close();
    }

    void TearDown() override {
        // Delete the temporary file
        std::filesystem::remove(temp_filename);
    }

    std::string temp_filename;
    std::ofstream temp_file;
};

TEST_F(FilePortionStreamFixture, ReadFilePortion) {
    std::streampos begin_pos = 0;
    std::streampos end_pos = 5;

    auto stream = FilePortionStream::open(temp_filename, begin_pos, end_pos);

    std::string portion;
    while (stream.good()) {
        auto c = stream.get();
        if (c == std::char_traits<char>::eof())
            break;
        portion.push_back(c);
    }

    EXPECT_EQ(portion, "Hello");
}

TEST_F(FilePortionStreamFixture, ReadMiddleOfFilePortion) {
    std::streampos begin_pos = 5;
    std::streampos end_pos = 12;

    auto stream = FilePortionStream::open(temp_filename, begin_pos, end_pos);

    std::string portion;
    while (stream.good()) {
        auto c = stream.get();
        if (c == std::char_traits<char>::eof())
            break;
        portion.push_back(c);
    }

    EXPECT_EQ(portion, ", World");
}

class EmptyFilePortionStreamFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a temporary file
        temp_filename =
            std::filesystem::temp_directory_path() / "temp_file.txt";
        temp_file.open(temp_filename);
        temp_file.close();
    }

    void TearDown() override {
        // Delete the temporary file
        std::filesystem::remove(temp_filename);
    }

    std::string temp_filename;
    std::ofstream temp_file;
};

TEST_F(EmptyFilePortionStreamFixture, ReadEmptyFilePortion) {
    std::streampos begin_pos = 0;
    std::streampos end_pos = 1000;

    auto stream = FilePortionStream::open(temp_filename, begin_pos, end_pos);

    std::string portion;
    std::getline(stream, portion);

    EXPECT_EQ(portion, "");
}