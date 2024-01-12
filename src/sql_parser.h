#pragma once

#include <absl/container/flat_hash_map.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "category_link_type.h"

namespace net_zelcon::wikidice {

class SQLParser {
  protected:
    std::istream &stream_;
    std::string table_name_;

  public:
    SQLParser(std::istream &stream, std::string_view table_name);

    void skip_header();

    auto next() -> std::optional<std::vector<std::string>>;

    auto with_stop_at(std::streamoff stop_at) -> SQLParser & {
        stop_at_ = stop_at;
        return *this;
    }

    static auto split_offsets(const std::filesystem::path &sqldump,
                              std::string_view table_name,
                              std::size_t n_partitions)
        -> std::vector<std::tuple<std::streamoff, std::streamoff>>;

  private:
    std::optional<std::streamoff> stop_at_;
};

class CategoryLinksParser : public SQLParser {
  public:
    CategoryLinksParser(std::istream &stream)
        : SQLParser(stream, "categorylinks") {}

    auto next() -> std::optional<CategoryLinksRow>;
};

class CategoryParser : public SQLParser {
  public:
    CategoryParser(std::istream &stream) : SQLParser(stream, "category") {}

    auto next() -> std::optional<CategoryRow>;
};

auto read_category_table(std::istream &stream)
    -> absl::flat_hash_map<std::uint64_t, CategoryRow>;

class FilePortionStream {
  private:
    class Buf : public std::filebuf {
      public:
        Buf(const std::filesystem::path &filename, std::streampos begin_pos,
            std::streampos end_pos)
            : end_pos_{end_pos} {
            open(filename, std::ios::in);
            pubseekpos(begin_pos);
        }
        int_type underflow() override {
            if (pubseekoff(0, std::ios::cur, std::ios::in) >= end_pos_)
                return traits_type::eof();
            if (gptr() < egptr())
                return traits_type::to_int_type(*gptr());
            return std::filebuf::underflow();
        }

      private:
        std::streampos end_pos_;
    };
    Buf *buf_ = nullptr;
    std::istream *stream_ = nullptr;

  public:
    FilePortionStream() = delete;

    FilePortionStream(const std::filesystem::path &filename,
                      std::streampos begin_pos, std::streampos end_pos) {
        buf_ = new Buf(filename, begin_pos, end_pos);
        stream_ = new std::istream(buf_);
        stream_->rdbuf(buf_);
    }

    ~FilePortionStream() {
        delete stream_;
        delete buf_;
    }

    auto get() -> std::istream & { return *stream_; }
};

auto split_sqldump(const std::filesystem::path &sqldump,
                   std::string_view table_name, uint32_t n_partitions)
    -> std::vector<std::unique_ptr<FilePortionStream>>;

} // namespace net_zelcon::wikidice