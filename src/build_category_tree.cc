#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/log/log.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/string_view.h>
#include <algorithm>
#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <locale>
#include <memory>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include "build_category_tree.h"
#include "category_table.h"
#include "category_tree_index.h"
#include "cross_platform_set_thread_name.h"
#include "sql_parser.h"

ABSL_FLAG(std::string, category_dump, "", "Path to the category SQL dump file");
ABSL_FLAG(std::string, categorylinks_dump, "",
          "Path to the categorylinks SQL dump file");
ABSL_FLAG(std::string, db_path, "", "Path to the database directory");
ABSL_FLAG(std::string, wikipedia_language_code, "en",
          "Wikipedia language code (e.g., \"en\", \"de\")");
ABSL_FLAG(uint32_t, threads, 8, "Number of threads to use");

namespace {

using namespace net_zelcon::wikidice;

auto read_category_table(const std::filesystem::path sqldump)
    -> std::shared_ptr<CategoryTable> {
    auto category_table = std::make_shared<CategoryTable>();
    uint64_t counter = 0;
    std::ifstream infile(sqldump);
    CategoryParser category_parser(infile);
    category_parser.skip_header();
    while (auto row = category_parser.next()) {
        category_table->add_category(row.value());
        if (++counter % 100'000 == 0) {
            LOG(INFO) << fmt::format("Read {} categories...", counter);
            LOG(INFO) << fmt::format("Last category: name={}, id={}",
                                     row.value().category_name,
                                     row->category_id);
        }
    }
    return category_table;
}

auto read_categorylinks_table(CategoryTreeIndexWriter &dst,
                              std::istream &table_stream, std::streamoff offset,
                              int thread, bool skip_header = false) -> void {
    CategoryLinksParser categorylinks_parser{table_stream};
    if (skip_header) {
        categorylinks_parser.skip_header();
    }
    if (offset != 0)
        categorylinks_parser.with_stop_at(offset);
    uint64_t counter = 0;
    while (auto row = categorylinks_parser.next()) {
        dst.import_categorylinks_row(row.value());
        if (++counter % 1'000'000 == 0)
            LOG(INFO) << fmt::format(
                "Thread #{} ({}): Imported {:L} rows. Last imported row: "
                "page_id={} (a {}) → category_name={}",
                thread, this_thread_name(), counter, row->page_id,
                to_string(row->page_type), row->category_name);
    }
}

auto parallel_read_categorylinks_table(CategoryTreeIndexWriter &dst,
                                       std::filesystem::path sqldump,
                                       uint32_t threads) -> void {
    auto offsets = SQLParser::split_offsets(sqldump, "categorylinks", threads);
    std::vector<std::thread> threads_running;
    for (uint32_t i = 0; i < offsets.size(); ++i) {
        threads_running.emplace_back([&sqldump, &dst, &offsets, i]() {
            LOG(INFO) << "Starting thread #" << i << "...";
            set_thread_name(fmt::format("thread #{}", i).c_str());
            std::ifstream stream{sqldump, std::ios::in};
            stream.seekg(std::get<0>(offsets[i]));
            read_categorylinks_table(dst, stream, std::get<1>(offsets[i]), i);
        });
    }
    for (auto &t : threads_running) {
        t.join();
    }
}

} // namespace

namespace net_zelcon::wikidice {

auto is_valid_language(std::string_view language) -> bool {
    return std::ranges::any_of(WIKIPEDIA_LANGUAGE_CODES, [language](auto code) {
        return code == language;
    });
}

} // namespace net_zelcon::wikidice

int main(int argc, char *argv[]) {
    using namespace net_zelcon::wikidice;
    try {
        // necessary for printing thousand separators in numbers
        std::locale l{"en_US.UTF-8"};
        std::locale::global(l);
    } catch (std::runtime_error &e) {
        LOG(WARNING) << "Failed to set locale to en_US.UTF-8: " << e.what();
    }
    absl::SetProgramUsageMessage(
        absl::StrCat("This program builds the initial database from Wikipedia "
                     "SQL dumps. Sample usage:\n",
                     argv[0]));
    absl::ParseCommandLine(argc, argv);
    CHECK(is_valid_language(absl::GetFlag(FLAGS_wikipedia_language_code)))
        << "Invalid Wikipedia language code (should be something like \"en\" "
           "or \"de\")";
    LOG(INFO) << "Reading category table from "
              << absl::GetFlag(FLAGS_category_dump) << "...";
    auto category_table =
        read_category_table(absl::GetFlag(FLAGS_category_dump));
    LOG(INFO) << "Done reading category table.";
    const auto db_destination_path =
        std::filesystem::path{absl::GetFlag(FLAGS_db_path)} /
        std::filesystem::path{absl::GetFlag(FLAGS_wikipedia_language_code)};
    CategoryTreeIndexWriter category_tree_index{db_destination_path,
                                                category_table};
    std::ifstream categorylinks_dump_stream{
        absl::GetFlag(FLAGS_categorylinks_dump)};
    LOG(INFO) << "Reading categorylinks table from "
              << absl::GetFlag(FLAGS_categorylinks_dump) << "...";
    parallel_read_categorylinks_table(category_tree_index,
                                      absl::GetFlag(FLAGS_categorylinks_dump),
                                      absl::GetFlag(FLAGS_threads));
    LOG(INFO) << "Done reading categorylinks table. Saved to: "
              << db_destination_path.string();
    LOG(INFO) << "Starting to build weights...";
    category_tree_index.run_second_pass();
    LOG(INFO) << "Done building weights. Saved to: "
              << db_destination_path.string();
    return 0;
}