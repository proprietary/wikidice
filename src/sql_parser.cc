#include "sql_parser.h"
#include "bounded_string_ring.h"
#include <absl/log/log.h>
#include <array>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <cctype>
#include <deque>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iostream>
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace net_zelcon::wikidice {

using std::literals::string_view_literals::operator""sv;

namespace {

void advance_to(std::istream &stream, std::string_view target) {
    const auto len = target.size();
    BoundedStringRing ring{len};
    char c;
    while (stream.good()) {
        c = stream.get();
        ring.push_back(c);
        if (ring == target) {
            break;
        }
    }
}

auto read_values_list(std::istream &stream) -> std::vector<std::string> {
    std::vector<std::string> dst;
    const auto expect_number = [&stream]() -> std::string {
        std::string result;
        char c = std::char_traits<char>::eof();
        while (stream.good()) {
            c = stream.get();
            if (c == '\n') {
                continue;
            } else if (c >= '0' && c <= '9') {
                result.push_back(c);
            } else {
                break;
            }
        }
        return result;
    };
    const auto expect_string = [&stream]() -> std::string {
        std::string result;
        char c = std::char_traits<char>::eof();
        while (stream.good()) {
            c = stream.get();
            if (!std::isprint(c))
                continue;
            if (c == '\\') {
                result.push_back(stream.get());
                continue;
            }
            if (c == '\'') {
                break;
            }
            result.push_back(c);
        }
        return result;
    };
    const auto advance_to_character = [&stream](const char target) {
        char c = std::char_traits<char>::eof();
        while (stream.good() && c != target) {
            c = stream.get();
        }
    };
    advance_to_character('(');
    while (stream.good()) {
        const char c = stream.peek();
        if (c == ')') {
            stream.get();
            break;
        } else if (c == ',' || c == '\n') {
            stream.get();
            continue;
        } else if (c == '\'') {
            stream.get();
            auto s = expect_string();
            dst.push_back(s);
        } else if (c >= '0' && c <= '9') {
            dst.emplace_back(expect_number());
        } else {
            break;
        }
    }
    return dst;
}

void unescape(std::string &) {
    // TODO
}

void remove_newlines(std::string &s) {
    const auto new_end = std::remove(s.begin(), s.end(), '\n');
    s.erase(new_end, s.end());
}

void sanitize(std::string &s) {
    remove_newlines(s);
    unescape(s);
}

} // namespace

SQLParser::SQLParser(std::istream &stream, std::string_view table_name)
    : stream_(stream), table_name_{table_name} {}

void SQLParser::skip_header() {
    const auto insert_into_statement =
        fmt::format("INSERT INTO `{}` VALUES ", table_name_);
    advance_to(stream_, insert_into_statement);
}

auto SQLParser::next() -> std::optional<std::vector<std::string>> {
    if (!stream_.good() || (stop_at_ && stream_.tellg() >= stop_at_.value())) {
        DLOG(WARNING) << "called next() on a bad stream";
        return std::nullopt;
    }
    std::vector<std::string> cols = read_values_list(stream_);
    if (cols.empty()) {
        return std::nullopt;
    }
    for (auto &col : cols) {
        sanitize(col);
    }
    return cols;
}

auto CategoryLinksParser::next() -> std::optional<CategoryLinksRow> {
    auto row = SQLParser::next();
    if (!row.has_value()) {
        return std::nullopt;
    }
    if (row->size() != 7) {
        int i = 0;
        while (i++ < 300)
            stream_.unget();
        i = 0;
        while (i++ < 1000) {
            std::cout << static_cast<char>(stream_.get());
        }
        std::cout << std::endl;
    }
    CHECK(row->size() == 7) << fmt::format("expected 7 columns, got {} in : {}",
                                           row->size(), row.value());
    CategoryLinksRow result;
    result.category_name = row->at(1);
    result.page_id = std::stoull(row->at(0));
    result.page_type = from(row->back());
    return result;
}

auto CategoryParser::next() -> std::optional<CategoryRow> {
    auto row = SQLParser::next();
    if (!row.has_value()) {
        return std::nullopt;
    }
    CHECK(row->size() == 5) << "row has " << row->size()
                            << " elements: " << fmt::format("{}", row.value());
    CategoryRow result;
    result.category_id = std::stoull(row->at(0));
    result.category_name = row->at(1);
    result.page_count = std::stoi(row->at(2));
    result.subcategory_count = std::stoi(row->at(3));
    return result;
}

auto read_category_table(std::istream &stream)
    -> absl::flat_hash_map<std::uint64_t, CategoryRow> {
    absl::flat_hash_map<std::uint64_t, CategoryRow> result;
    CategoryParser parser{stream};
    while (true) {
        auto row = parser.next();
        if (!row.has_value()) {
            break;
        }
        result[row->category_id] = *row;
    }
    return result;
}

auto split_sqldump(const std::filesystem::path &sqldump,
                   std::string_view table_name, uint32_t n_partitions)
    -> std::vector<std::unique_ptr<FilePortionStream>> {
    CHECK(std::filesystem::is_regular_file(sqldump));
    auto parts = SQLParser::split_offsets(sqldump, table_name, n_partitions);
    std::vector<std::unique_ptr<FilePortionStream>> dst;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        dst.emplace_back(std::make_unique<FilePortionStream>(
            sqldump, std::get<0>(parts[i]), std::get<1>(parts[i])));
    }
    return dst;
}

auto SQLParser::split_offsets(const std::filesystem::path &sqldump,
                              std::string_view table_name, size_t n_partitions)
    -> std::vector<std::tuple<std::streamoff, std::streamoff>> {
    CHECK_GT(n_partitions, 0ULL);
    CHECK(std::filesystem::is_regular_file(sqldump));
    std::vector<std::tuple<std::streamoff, std::streamoff>> result;
    std::ifstream stream{sqldump, std::ios::in};
    SQLParser parser{stream, table_name};
    parser.skip_header();
    std::streamoff begin_pos = stream.tellg();
    const auto filesize = std::filesystem::file_size(sqldump);
    const auto partition_size =
        filesize / static_cast<std::uintmax_t>(n_partitions);
    std::streamoff nth_partition = 1;
    while (parser.next()) {
        std::streamoff current_pos = stream.tellg();
        if (static_cast<uintmax_t>(current_pos) >=
            (partition_size * nth_partition)) {
            result.emplace_back(begin_pos, current_pos);
            begin_pos = current_pos;
            nth_partition++;
        }
    }
    result.emplace_back(begin_pos, filesize);
    return result;
}

} // namespace net_zelcon::wikidice