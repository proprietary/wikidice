#pragma once

#include <algorithm>
#include <array>
#include <fstream>
#include <streambuf>
#include <utility>

namespace net_zelcon::wikidice::file_utils {

/**
 * @brief A stream that reads a portion of a file. This exists mainly to aid in
 * reading a large SQL dump in parallel from many threads.
 */
class FilePortionStream : public std::istream {
  private:
    /**
     * @brief A stream buffer wrapping another stream buffer, but only allowing
     * reads from a specified range of the underlying stream.
     */
    class RangedStreamBuf : public std::streambuf {
      public:
        RangedStreamBuf(std::filebuf *buf, std::streampos begin,
                        std::streampos end)
            : wrapped_buf_{buf}, begin_{begin}, end_{end} {
            wrapped_buf_->pubseekoff(begin, std::ios::beg, std::ios::in);
            this->setg(nullptr, nullptr, nullptr);
        }

        int_type underflow() override {
            if (this->gptr() >= this->egptr()) {
                auto current_pos =
                    wrapped_buf_->pubseekoff(0, std::ios::cur, std::ios::in);
                if (current_pos >= end_)
                    return traits_type::eof();
                auto read_up_to =
                    std::min(static_cast<std::intmax_t>(current_pos) +
                                 static_cast<std::intmax_t>(BUFFER_SIZE),
                             static_cast<std::intmax_t>(end_));
                auto expected_count = read_up_to - current_pos;
                auto got_count = wrapped_buf_->sgetn(buffer_.data(), expected_count);
                if (got_count == 0)
                    return traits_type::eof();
                setg(buffer_.data(), buffer_.data(),
                     buffer_.data() + got_count);
            }
            return std::char_traits<char>::to_int_type(*gptr());
        }

      private:
        std::filebuf *wrapped_buf_;
        std::streampos begin_;
        std::streampos end_;
        constexpr static std::size_t BUFFER_SIZE = 1 << 20; // 1 MiB
        std::array<char, BUFFER_SIZE> buffer_;
    };

    std::unique_ptr<std::ifstream> file_stream_;

    FilePortionStream(std::filebuf *fbuf, std::streampos begin_pos,
                      std::streampos end_pos)
        : std::istream(new RangedStreamBuf(fbuf, begin_pos, end_pos)) {}

  public:
    FilePortionStream() = delete;

    [[nodiscard]] static auto
    open(const std::filesystem::path &filename, std::streampos begin_pos,
         std::streampos end_pos) -> FilePortionStream {
        std::unique_ptr<std::ifstream> file_stream{
            new std::ifstream{filename, std::ios::in}};
        FilePortionStream dst{file_stream->rdbuf(), begin_pos, end_pos};
        dst.file_stream_ = std::move(file_stream);
        return dst;
    }

    FilePortionStream(FilePortionStream &&other) : std::istream(other.rdbuf()) {
        other.rdbuf(nullptr);
        file_stream_ = std::move(other.file_stream_);
    }

    FilePortionStream &operator=(FilePortionStream &&other) {
        if (rdbuf() != nullptr)
            delete rdbuf();
        rdbuf(other.rdbuf());
        other.rdbuf(nullptr);
        file_stream_ = std::move(other.file_stream_);
        return *this;
    }

    ~FilePortionStream() {
        if (rdbuf() != nullptr)
            delete rdbuf();
    }
};

auto get_file_size(std::istream &stream) -> std::streamoff;

} // namespace net_zelcon::wikidice::file_utils