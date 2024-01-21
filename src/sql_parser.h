#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/log/check.h>
#include <absl/log/log.h>
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
#include <thread>
#include <utility>

#include "entities/entities.h"
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
    auto get_table_name() const -> std::string_view { return table_name_; }

    auto split_offsets(uint32_t n_partitions)
        -> std::vector<std::pair<std::streamoff, std::streamoff>>;

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
    using strategy_type = T;

    SQLParser(std::istream &stream, std::string_view table_name)
        : SQLDumpParserUntypedRows{stream, table_name} {}

    auto next() -> std::optional<typename T::column_type>;
};

class CategoryLinksRowDecompositionStrategy {
  public:
    using column_type = entities::CategoryLinksRow;
    static constexpr std::string_view table_name = "categorylinks"sv;

    static auto decompose(const std::vector<std::string> &row)
        -> entities::CategoryLinksRow;
};

class CategoryTableRowDecompositionStrategy {
  public:
    using column_type = entities::CategoryRow;
    static constexpr std::string_view table_name = "category"sv;

    static auto
    decompose(const std::vector<std::string> &row) -> entities::CategoryRow;
};

class PageRowDecompositionStrategy {
  public:
    using column_type = entities::PageTableRow;
    static constexpr std::string_view table_name = "page"sv;

    static auto
    decompose(const std::vector<std::string> &row) -> entities::PageTableRow;
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
    -> absl::flat_hash_map<std::uint64_t, entities::CategoryRow>;

template <ColumnDecompositionStrategy T>
auto SQLParser<T>::next() -> std::optional<typename T::column_type> {
    auto cols = SQLDumpParserUntypedRows::next();
    if (!cols || cols->size() == 0)
        return std::nullopt;
    return T::decompose(cols.value());
}

template <typename SQLParserType,
          ColumnDecompositionStrategy C = SQLParserType::strategy_type>
    requires std::is_base_of_v<SQLParser<C>, SQLParserType> &&
             requires(SQLParserType a) {
                 {
                     SQLParserType::table_name
                 } -> std::convertible_to<std::string_view>;
             }
class SQLDumpParallelProcessor {
  private:
    std::filesystem::path sqldump_;
    std::string table_name_;
    uint32_t n_threads_;

    auto make_parallel()
        -> std::vector<std::unique_ptr<file_utils::FilePortionStream>> {
        using file_utils::FilePortionStream;
        std::ifstream ps{sqldump_, std::ios::in};
        SQLParserType p{ps};
        auto offsets = p.split_offsets(n_threads_);
        std::vector<std::unique_ptr<FilePortionStream>> dst;
        for (std::size_t i = 0; i < offsets.size(); ++i) {
            auto &[begin, end] = offsets[i];
            dst.emplace_back(std::make_unique<FilePortionStream>(
                FilePortionStream::open(sqldump_, begin, end)));
        }
        return dst;
    }

  public:
    explicit SQLDumpParallelProcessor(const std::filesystem::path &sqldump,
                                      uint32_t n_threads)
        : sqldump_{sqldump}, table_name_{SQLParserType::table_name},
          n_threads_{n_threads} {
        CHECK_GT(n_threads, 0U);
        CHECK(std::filesystem::is_regular_file(sqldump_));
    }

    explicit SQLDumpParallelProcessor(const std::filesystem::path &sqldump)
        : SQLDumpParallelProcessor{sqldump,
                                   std::thread::hardware_concurrency()} {}

    auto set_parallelism(uint32_t n_threads) noexcept -> void {
        CHECK_GT(n_threads, 0U);
        n_threads_ = n_threads;
    }

    template <typename Fn> auto operator()(Fn &&fn) -> void {
        auto streams = make_parallel();
        std::vector<std::thread> threads;
        for (uint32_t thread_num = 0; thread_num < streams.size();
             ++thread_num) {
            threads.emplace_back([&streams, thread_num, &fn]() {
                LOG(INFO) << "Starting thread #" << thread_num << "...";
                std::istream &stream = *streams[thread_num];
                SQLParserType parser{stream};
                fn(parser);
            });
        }
        for (auto &t : threads)
            t.join();
    }
};

} // namespace net_zelcon::wikidice