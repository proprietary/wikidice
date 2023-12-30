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
#include <array>

namespace net_zelcon::wikidice {

using std::literals::string_view_literals::operator""sv;

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

template<std::size_t N>
auto read_until(std::istream& stream, const std::array<std::string_view, N>& terminators) -> std::tuple<std::string, std::size_t> {
    std::string result;
    char c = stream.get();
    while (stream.good()) {
        result.push_back(c);
        for (std::size_t i = 0; i < terminators.size(); ++i) {
            const auto& target = terminators[i];
            const auto len = target.length();
            if (result.length() >= len && std::equal(result.begin() + result.size() - len, result.end(), target.begin())) {
                result.erase(result.length() - len, len);
                return {result, i};
            }
        }
        c = stream.get();
    }
    LOG(FATAL) << "unexpected end of file";
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
                if (i > 0 && src[i - 1] == '\\') {
                    result.back() = '\'';
                    i++;
                    continue;
                }
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
    const auto insert_into_statement = fmt::format("INSERT INTO `{}` VALUES (", table_name_);
    advance_to(stream_, insert_into_statement);
}

auto SQLParser::next() -> std::optional<std::vector<std::string>> {
    if (!stream_.good()) {
        DLOG(WARNING) << "called next() on a bad stream";
        return std::nullopt;
    }
    constexpr std::array terminators = {"),("sv, ");\n"sv};
    auto [row_str, which_terminator] = read_until(stream_, terminators);
    if (row_str.empty()) {
        DLOG(WARNING) << "unexpected end of file";
        return std::nullopt;
    }
    if (which_terminator == 1) {
        // advance to the next insert statement
        const auto insert_into_statement = fmt::format("INSERT INTO `{}` VALUES (", table_name_);
        advance_to(stream_, insert_into_statement);
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