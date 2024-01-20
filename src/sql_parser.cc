#include "sql_parser.h"
#include "bounded_string_ring.h"
#include "file_utils/file_utils.h"
#include <absl/log/check.h>
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
#include <utility>
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
            } else if ((c >= '0' && c <= '9') || (c == '.')) {
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
    const auto expect_null = [&stream]() -> bool {
        const auto null_literal = "NULL"sv;
        for (size_t i = 0; i < null_literal.length(); ++i) {
            CHECK(stream.good());
            if (stream.get() != null_literal[i]) {
                LOG(ERROR) << "Expected NULL literal, but got something else.";
                // unwind
                for (size_t j = 0; j < i; ++j) {
                    stream.unget();
                }
                return false;
            }
        }
        return true;
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
        } else if (c == 'N') {
            bool ok = expect_null();
            if (ok)
                dst.push_back("NULL");
            else
                LOG(FATAL) << "Expected NULL literal, but got something else.";
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

auto SQLDumpParserUntypedRows::next()
    -> std::optional<std::vector<std::string>> {
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

auto SQLDumpParserUntypedRows::insert_into_statement() const -> std::string {
    return fmt::format("INSERT INTO `{}` VALUES ", table_name_);
}

void SQLDumpParserUntypedRows::skip_header() {
    advance_to(stream_, insert_into_statement());
}

auto SQLDumpParserUntypedRows::split_offsets(uint32_t n_partitions)
    -> std::vector<std::pair<std::streamoff, std::streamoff>> {
    CHECK_GT(n_partitions, 0ULL);
    const auto original_pos = stream_.tellg();
    std::vector<std::pair<std::streamoff, std::streamoff>> result;
    const std::intmax_t filesize = file_utils::get_file_size(stream_);
    const std::intmax_t partition_size =
        filesize / static_cast<std::intmax_t>(n_partitions);
    advance_to(stream_, insert_into_statement());
    auto pos = stream_.tellg();
    while (stream_.good()) {
        if (static_cast<std::intmax_t>(pos) + partition_size >= filesize) {
            result.emplace_back(pos, filesize);
            break;
        }
        stream_.seekg(static_cast<std::intmax_t>(pos) + partition_size);
        advance_to(stream_, insert_into_statement());
        if (!stream_.good()) {
            result.emplace_back(pos, filesize);
        }
        std::streamoff current_pos = stream_.tellg();
        result.emplace_back(pos, current_pos);
        pos = current_pos;
    }
    // restore original position
    stream_.seekg(original_pos);

    return result;
}

auto CategoryLinksRowDecompositionStrategy::decompose(
    const std::vector<std::string> &row) -> entities::CategoryLinksRow {
    CHECK_EQ(7ULL, row.size())
        << fmt::format("expected 7 columns, got {} in : {}", row.size(), row);
    entities::CategoryLinksRow result;
    result.category_name = row.at(1);
    result.page_id = std::stoull(row.at(0));
    result.page_type = entities::from(row.back());
    return result;
}

auto CategoryTableRowDecompositionStrategy::decompose(
    const std::vector<std::string> &row) -> entities::CategoryRow {
    CHECK_EQ(5ULL, row.size())
        << "row has " << row.size() << " elements: " << fmt::format("{}", row);
    entities::CategoryRow result;
    result.category_id = std::stoull(row.at(0));
    result.category_name = row.at(1);
    result.page_count = std::stoi(row.at(2));
    result.subcategory_count = std::stoi(row.at(3));
    return result;
}

auto PageRowDecompositionStrategy::decompose(
    const std::vector<std::string> &row) -> entities::PageTableRow {
    CHECK_EQ(12ULL, row.size())
        << fmt::format("Expected 12 rows in page table row, but got {} in: {}",
                       row.size(), row);
    CHECK(row.at(3) == "0" || row.at(3) == "1") << fmt::format(
        "Expected 0 or 1 in page table row, but got {} in: {}", row.at(3), row);
    entities::PageTableRow result;
    result.is_redirect = row.at(3) == "1";
    result.page_id = std::stoull(row.at(0));
    result.page_title = row.at(2);
    return result;
}

auto read_category_table(std::istream &stream)
    -> absl::flat_hash_map<std::uint64_t, entities::CategoryRow> {
    absl::flat_hash_map<std::uint64_t, entities::CategoryRow> result;
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