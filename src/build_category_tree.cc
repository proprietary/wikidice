#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/log/check.h>
#include <absl/log/log.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/string_view.h>
#include <algorithm>
#include <atomic>
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
#include "wiki_page_table.h"

ABSL_FLAG(
    std::string, category_dump, "",
    "Path to the category SQL dump file. Example: enwiki-latest-category.sql");
ABSL_FLAG(std::string, categorylinks_dump, "",
          "Path to the categorylinks SQL dump file. Example: "
          "enwiki-latest-categorylinks.sql");
ABSL_FLAG(std::string, page_dump, "",
          "Path to the page SQL dump file. Example: enwiki-latest-page.sql");
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
        LOG_IF(INFO, ++counter % 100'000 == 0)
            << "Read " << counter
            << " categories. Last category: name=" << row.value().category_name
            << ", id=" << row->category_id;
    }
    return category_table;
}

auto parallel_import_categorylinks(CategoryTreeIndexWriter &dst,
                                   std::filesystem::path categorylinks_dump,
                                   uint32_t n_threads) -> void {
    LOG(INFO) << "Importing categorylinks table into RocksDB database at "
              << std::quoted(categorylinks_dump.string()) << " using "
              << n_threads << " threads...";
    std::atomic<uint64_t> counter{0};
    uint32_t thread_num = 0;
    SQLDumpParallelProcessor<CategoryLinksParser> parallel_processor(
        categorylinks_dump);
    parallel_processor.set_parallelism(n_threads);
    parallel_processor([&dst, &counter, nth_thread = thread_num++](
                           CategoryLinksParser &parser) {
        LOG(INFO) << "Starting thread #" << nth_thread << "...";
        static constexpr size_t batch_size = 10ULL;
        std::vector<CategoryLinksRow> batch;
        batch.reserve(batch_size);
        while (std::optional<CategoryLinksRow> row = parser.next()) {
            // Enqueue row to write batch
            batch.emplace_back(row.value());

            // Report progress
            auto count_result = counter.fetch_add(1);
            LOG_IF(INFO, count_result % 1'000'000 == 0)
                << "Thread #" << nth_thread << " imported " << count_result
                << " rows."
                << " Last imported row: page_id=" << batch.back().page_id
                << " (a " << to_string(batch.back().page_type)
                << ") → category_name=" << batch.back().category_name;

            // Actualy insert batch into database if necessary
            if (batch.size() >= batch_size) {
                DLOG(INFO) << "Thread #" << nth_thread << " importing batch of "
                           << batch.size() << " rows...";
                dst.import_categorylinks_rows(batch);
                batch.clear();
            }
        }
        if (!batch.empty()) {
            dst.import_categorylinks_rows(batch);
        }
    });
}

auto parallel_import_page_table(std::shared_ptr<WikiPageTable> dst,
                                std::filesystem::path page_dump,
                                uint32_t n_threads) -> void {
    LOG(INFO) << "Reading page table from " << std::quoted(page_dump.string())
              << " using " << n_threads << " threads ...";
    std::atomic<uint64_t> counter{0};
    std::atomic<uint32_t> thread_num{0};
    SQLDumpParallelProcessor<PageTableParser> parallel_processor(page_dump);
    parallel_processor.set_parallelism(n_threads);
    parallel_processor([&dst, &counter, &thread_num](PageTableParser &parser) {
        LOG(INFO) << "Starting thread #" << thread_num.fetch_add(1) << "...";
        while (std::optional<PageTableRow> row = parser.next()) {
            dst->add_page(row.value());
            auto count_result = counter.fetch_add(1);
            LOG_IF(INFO, count_result % 1'000'000 == 0)
                << "Thread #" << thread_num << " imported " << count_result
                << " rows." << " Last imported row: page_id=" << row->page_id
                << " → page_title=" << row->page_title;
        }
    });
}

} // namespace

namespace net_zelcon::wikidice {

auto is_valid_language(std::string_view language) -> bool {
    return std::find(std::begin(WIKIPEDIA_LANGUAGE_CODES),
                     std::end(WIKIPEDIA_LANGUAGE_CODES),
                     language) != std::end(WIKIPEDIA_LANGUAGE_CODES);
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
        absl::StrCat("This program builds the initial database from Wikipedia"
                     "SQL dumps. Sample usage:\n",
                     argv[0]));
    absl::ParseCommandLine(argc, argv);
    CHECK(is_valid_language(absl::GetFlag(FLAGS_wikipedia_language_code)))
        << "Invalid Wikipedia language code (should be something like \"en\" "
           "or \"de\")";

    // Import category table
    LOG(INFO) << "Reading category table from "
              << absl::GetFlag(FLAGS_category_dump) << "...";
    auto category_table =
        read_category_table(absl::GetFlag(FLAGS_category_dump));
    LOG(INFO) << "Done reading category table.";

    // Import page table
    const auto page_dump_db_path =
        std::filesystem::path{absl::GetFlag(FLAGS_db_path)} /
        std::filesystem::path{absl::StrCat(
            absl::GetFlag(FLAGS_wikipedia_language_code), "_pages")};
    LOG(INFO) << "Creating temporary database for the `page` table at "
              << page_dump_db_path.string() << "...";
    auto page_table = std::make_shared<WikiPageTable>(page_dump_db_path);
    parallel_import_page_table(page_table, absl::GetFlag(FLAGS_page_dump),
                               absl::GetFlag(FLAGS_threads));

    // Import categorylinks table
    const auto db_destination_path =
        std::filesystem::path{absl::GetFlag(FLAGS_db_path)} /
        std::filesystem::path{absl::GetFlag(FLAGS_wikipedia_language_code)};
    {
        CategoryTreeIndexWriter category_tree_index{
            db_destination_path, category_table, page_table,
            absl::GetFlag(FLAGS_threads)};
        LOG(INFO) << "Reading categorylinks table from "
                  << absl::GetFlag(FLAGS_categorylinks_dump) << "...";
        parallel_import_categorylinks(category_tree_index,
                                      absl::GetFlag(FLAGS_categorylinks_dump),
                                      absl::GetFlag(FLAGS_threads));
        LOG(INFO) << "Done reading categorylinks table. Saved to: "
                  << db_destination_path.string();
        LOG(INFO) << "Starting to build weights...";
        category_tree_index.run_second_pass();
        LOG(INFO) << "Done building weights. Saved to: "
                  << db_destination_path.string();
        // Removing temporary database for the `page` table
        page_table.reset();
        std::filesystem::remove_all(page_dump_db_path);
        LOG(INFO) << "Removed temporary database for the `page` table at "
                  << page_dump_db_path.string();
    }
    // Prepare database for reads and make sure it works
    LOG(INFO) << "Compressing database to ready it for reads...";
    CategoryTreeIndexReader category_tree_index_reader{db_destination_path};
    category_tree_index_reader.run_compaction();
    // Sanity check
    const auto rows = category_tree_index_reader.count_rows();
    LOG(INFO) << "Built database with " << rows << " rows.";
    return 0;
}