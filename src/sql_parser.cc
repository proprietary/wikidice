#include "sql_parser.h"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <istream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <deque>
#include <vector>
#include "bounded_string_ring.h"
#include <boost/algorithm/string/classification.hpp>
#include <absl/log/log.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

namespace net_zelcon::wikidice {

using std::literals::string_view_literals::operator""sv;

// ParsedRowIter::ParsedRowIter(std::istream& stream) : stream_{stream} {
//     // advance stream to the "insert into..." statement
//     constexpr auto insert_into_statement =
//         R"(INSERT INTO `categorylinks` VALUES )"sv;
//     BoundedStringRing read_buffer(insert_into_statement.size());
//     char c;
//     while ((c = stream_.get()) != std::char_traits<char>::eof()) {
//         read_buffer.push_back(c);
//         if (read_buffer == insert_into_statement) {
//             break;
//         }
//     }
// }

// auto ParsedRowIter::next()
//     -> std::optional<std::reference_wrapper<ParsedRowIter>> {
//     // read until the next row
//     constexpr char row_start = '(';
//     char c;
//     while ((c = stream_.get()) != std::char_traits<char>::eof()) {
//         if (c == row_start) {
//             break;
//         }
//     }
//     if (stream_.eof()) {
//         done_ = true;
//         spdlog::debug("unexpected end of file #1");
//         return std::nullopt;
//     }

//     // read until the end of the row
//     std::string row_str;
//     constexpr auto row_end = "),"sv;
//     constexpr auto statement_end = ");"sv;
//     constexpr std::array terminators = {row_end, statement_end};
//     BoundedStringRing read_buffer(row_end.size());
//     while ((c = stream_.get()) != std::char_traits<char>::eof()) {
//         row_str.push_back(c);
//         read_buffer.push_back(c);
//         auto found_terminator = std::find(terminators.begin(), terminators.end(), read_buffer);
//         if (found_terminator != terminators.end()) {
//             row_str.erase(row_str.size() - found_terminator->size(), found_terminator->size());
//             break;
//         }
//     }
//     if (stream_.eof()) {
//         done_ = true;
//         spdlog::debug("unexpected end of file #2");
//         return std::nullopt;
//     }

//     std::vector<std::string> cols;
//     boost::algorithm::split(cols, row_str, boost::is_any_of(","));
//     for (auto &col : cols) {
//         if (col.front() == '\'') {
//             col.erase(0, 1);
//         }
//         if (col.back() == '\'') {
//             col.erase(col.size() - 1, 1);
//         }
//         const auto new_end = std::remove(col.begin(), col.end(), '\n');
//         col.erase(new_end, col.end());
//     }

//     constexpr auto expected_columns = 7;
//     if (cols.size() < expected_columns) {
//         spdlog::error("expected {} columns here: {}", expected_columns,
//                       fmt::join(cols, ", "));
//         return std::nullopt;
//     }

//     row_.set_page_id(std::stoull(cols[0]));
//     row_.set_category_name(cols[1]);
//     row_.set_category_link_type(from(cols.back()));

//     return *this;
// }

// auto ParsedRowIter::get() const noexcept -> const Row & { return this->row_; }

void advance_to(std::istream& stream, std::string_view target) {
    const auto len = target.size();
    BoundedStringRing ring{len};
    char c;
    while ((c = stream.get()) != std::char_traits<char>::eof()) {
        ring.push_back(c);
        if (ring == target) {
            break;
        }
    }
}

auto read_until(std::istream& stream, std::string_view target) -> std::string {
    std::string result;
    const auto len = target.size();
    char c;
    while ((c = stream.get()) != std::char_traits<char>::eof()) {
        result.push_back(c);
        if (result.size() >= len && std::equal(result.begin() + result.size() - len, result.end(), target.begin())) {
            break;
        }
    }
    result.erase(result.size() - len, len);
    return result;
}

auto split_row(std::string_view src) -> std::vector<std::string> {
    std::vector<std::string> dst;
    size_t i = 0;
    auto read_number = [&src, &i]() -> std::string {
        std::string result;
        while (i < src.length()) {
            if (src[i] == '\n') {
                ++i;
                continue;
            } else if (src[i] >= '0' && src[i] <= '9') {
                result.push_back(src[i]);
                ++i;
            } else {
                break;
            }
        }
        return result;
    };
    auto read_string = [&src, &i]() -> std::string {
        std::string result;
        while (i < src.length()) {
            if (src[i] == '\'') {
                if (i > 0 && src[i - 1] == '\\')
                    continue;
                break;
            }
            result.push_back(src[i]);
            ++i;
        }
        return result;
    };
    auto read_comma = [&src, &i]() {
        while (i < src.length() && src[i] != ',') {
            ++i;
        }
        ++i;
    };
    while (i < src.length()) {
        if (src[i] == '\'') {
            ++i;
            dst.push_back(read_string());
            read_comma();
        } else if (src[i] >= '0' && src[i] <= '9') {
            dst.push_back(read_number());
            read_comma();
        } else {
            ++i;
        }
    }
    return dst;
}

SQLParser::SQLParser(std::istream& stream, std::string_view table_name): stream_(stream), table_name_{table_name} {
    // advance stream to the "insert into..." statement
    const auto insert_into_statement = fmt::format("INSERT INTO `{}` VALUES ", table_name_);
    advance_to(stream_, insert_into_statement);
}

auto SQLParser::next() -> std::optional<std::vector<std::string>> {
    constexpr auto row_start = "("sv;
    constexpr auto row_end = "),"sv;
    if (!stream_.good()) {
        DLOG(WARNING) << "called next() on a bad stream";
        spdlog::debug("called next() on a bad stream");
        return std::nullopt;
    }
    if (stream_.peek() != row_start.front()) {
        advance_to(stream_, row_start);
    }
    if (stream_.eof()) {
        DLOG(WARNING) << "unexpected end of file";
        return std::nullopt;
    }
    stream_.get();
    std::string row_str = read_until(stream_, row_end);
    if (row_str.empty()) {
        DLOG(WARNING) << "unexpected end of file";
        return std::nullopt;
    }
    std::vector<std::string> cols = split_row(row_str);
    for (auto &col : cols) {
        if (col.front() == '\'') {
            col.erase(0, 1);
        }
        if (col.back() == '\'') {
            col.erase(col.size() - 1, 1);
        }
        const auto new_end = std::remove(col.begin(), col.end(), '\n');
        col.erase(new_end, col.end());
    }
    return cols;
}

auto CategoryLinksParser::next() -> std::optional<CategoryLinksRow> {
    auto row = SQLParser::next();
    if (!row.has_value()) {
      return std::nullopt;
    }
    CHECK(row->size() == 7) << fmt::format("expected 7 columns, got {} in : {}", row->size(), row.value());
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
    CHECK(row->size() == 5);
    CategoryRow result;
    result.category_id = std::stoull(row->at(0));
    result.category_name = row->at(1);
    result.page_count = std::stoi(row->at(2));
    result.subcategory_count = std::stoi(row->at(3));
    return result;
}

auto read_category_table(std::istream& stream) -> absl::flat_hash_map<std::uint64_t, CategoryRow> {
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

} // namespace net_zelcon::wikidice