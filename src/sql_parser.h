#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/log/check.h>
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
#include <streambuf>
#include <string>
#include <string_view>
#include <utility>

#include "category_link_type.h"
#include "file_utils/file_utils.h"

namespace net_zelcon::wikidice {

using std::literals::string_view_literals::operator""sv;

class SQLDumpParserUntypedRows {
  public:
    SQLDumpParserUntypedRows(std::istream &stream, std::string_view table_name)
        : stream_{stream}, table_name_{table_name} {}
    auto next() -> std::optional<std::vector<std::string>>;
    auto with_stop_at(std::streamoff stop_at) -> SQLDumpParserUntypedRows & {
        stop_at_ = stop_at;
        return *this;
    }
    auto skip_header() -> void;

  protected:
    auto split_offsets(std::size_t n_partitions)
        -> std::vector<std::tuple<std::streamoff, std::streamoff>>;

  private:
    auto insert_into_statement() const -> std::string;
    std::istream &stream_;
    std::string table_name_;
    std::optional<std::streamoff> stop_at_;
};

template <typename T>
concept ColumnDecompositionStrategy =
    requires(T t, const std::vector<std::string> &row) {
        typename T::column_type;
        { T::decompose(row) } -> std::convertible_to<typename T::column_type>;
    };

class BasicColumnDecompositionStrategy {
  public:
    using column_type = std::vector<std::string>;

    static auto decompose(const std::vector<std::string> &row) -> column_type {
        return row;
    }
};

template <ColumnDecompositionStrategy T = BasicColumnDecompositionStrategy>
class SQLParser : public SQLDumpParserUntypedRows {
  public:
    SQLParser(std::istream &stream, std::string_view table_name)
        : SQLDumpParserUntypedRows{stream, table_name} {}

    auto next() -> std::optional<typename T::column_type>;

    static auto make_parallel(const std::filesystem::path &sqldump,
                              std::string_view table_name,
                              std::size_t n_partitions)
        -> std::vector<std::pair<
            SQLParser<T>, std::unique_ptr<file_utils::FilePortionStream>>>;
};

class CategoryLinksRowDecompositionStrategy {
  public:
    using column_type = CategoryLinksRow;
    static constexpr std::string_view table_name = "categorylinks"sv;

    static auto
    decompose(const std::vector<std::string> &row) -> CategoryLinksRow;
};

class CategoryTableRowDecompositionStrategy {
  public:
    using column_type = CategoryRow;
    static constexpr std::string_view table_name = "category"sv;

    static auto decompose(const std::vector<std::string> &row) -> CategoryRow;
};

class PageRowDecompositionStrategy {
  public:
    using column_type = PageTableRow;
    static constexpr std::string_view table_name = "page"sv;

    static auto decompose(const std::vector<std::string> &row) -> PageTableRow;
};

class CategoryLinksParser
    : public SQLParser<CategoryLinksRowDecompositionStrategy> {
  public:
    constexpr static std::string_view table_name =
        CategoryLinksRowDecompositionStrategy::table_name;
    CategoryLinksParser(std::istream &stream) : SQLParser(stream, table_name) {}
};

class PageTableParser : public SQLParser<PageRowDecompositionStrategy> {
  public:
    constexpr static std::string_view table_name =
        PageRowDecompositionStrategy::table_name;
    PageTableParser(std::istream &stream) : SQLParser(stream, table_name) {}
};

class CategoryParser : public SQLParser<CategoryTableRowDecompositionStrategy> {
  public:
    constexpr static std::string_view table_name =
        CategoryTableRowDecompositionStrategy::table_name;
    CategoryParser(std::istream &stream) : SQLParser(stream, table_name) {}
};

auto read_category_table(std::istream &stream)
    -> absl::flat_hash_map<std::uint64_t, CategoryRow>;

template <ColumnDecompositionStrategy T>
auto SQLParser<T>::next() -> std::optional<typename T::column_type> {
    auto cols = SQLDumpParserUntypedRows::next();
    if (!cols || cols->size() == 0)
        return std::nullopt;
    return T::decompose(cols.value());
}

template <ColumnDecompositionStrategy T>
auto SQLParser<T>::make_parallel(const std::filesystem::path &sqldump,
                                 std::string_view table_name,
                                 std::size_t n_partitions)
    -> std::vector<std::pair<SQLParser<T>,
                             std::unique_ptr<file_utils::FilePortionStream>>> {
    using file_utils::FilePortionStream;
    CHECK_GT(n_partitions, 0ULL);
    CHECK(std::filesystem::is_regular_file(sqldump));
    std::ifstream ps{sqldump, std::ios::in};
    SQLParser<T> p{ps, table_name};
    auto offsets = p.split_offsets(n_partitions);
    std::vector<std::pair<SQLParser<T>, std::unique_ptr<FilePortionStream>>>
        dst;
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        auto stream =
            std::make_unique<FilePortionStream>(FilePortionStream::open(
                sqldump, std::get<0>(offsets[i]), std::get<1>(offsets[i])));
        dst.emplace_back(std::make_pair(SQLParser<T>{*stream, table_name},
                                        std::move(stream)));
    }
    return dst;
}

} // namespace net_zelcon::wikidice