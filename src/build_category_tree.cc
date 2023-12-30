#include <absl/flags/commandlineflag.h>
#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/flags/usage_config.h>
#include <absl/log/log.h>
#include <absl/time/clock.h>
#include <absl/time/time.h>
#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <memory>
#include <regex>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#include "category_table.h"
#include "category_tree_index.h"
#include "sql_parser.h"

ABSL_FLAG(std::string, category_dump, "enwiki-20231201-category.sql",
          "Path to the category SQL dump file");
ABSL_FLAG(std::string, categorylinks_dump, "enwiki-20231201-categorylinks.sql",
          "Path to the categorylinks SQL dump file");
ABSL_FLAG(std::string, db_path, "/tmp/wikidice.db",
          "Path to the database directory");

namespace net_zelcon::wikidice {

namespace {

auto read_category_table(const std::filesystem::path sqldump)
    -> std::shared_ptr<CategoryTable> {
    auto category_table = std::make_shared<CategoryTable>();
    uint64_t counter = 0;
    std::ifstream infile(sqldump);
    CategoryParser category_parser(infile);
    while (auto row = category_parser.next()) {
        category_table->add_category(row.value());
        if (++counter % 10000 == 0) {
            LOG(INFO) << fmt::format("Read {} categories...", counter);
            LOG(INFO) << fmt::format("Last category: name={}, id={}",
                                     row.value().category_name,
                                     row->category_id);
        }
    }
    return category_table;
}

auto read_categorylinks_table(CategoryTreeIndex &dst,
                              const std::filesystem::path sqldump) -> void {
    std::ifstream infile{sqldump};
    CategoryLinksParser categorylinks_parser{infile};
    while (auto row = categorylinks_parser.next()) {
        DLOG(INFO) << "Imported row: " << row->page_id << " -> "
                   << row->category_name << " (" << to_string(row->page_type)
                   << ")";
        dst.import_categorylinks_row(row.value());
    }
}

} // namespace

} // namespace net_zelcon::wikidice

auto main(int argc, char **argv) -> int {
    using namespace net_zelcon::wikidice;
    absl::SetProgramUsageMessage(
        "Builds a category tree index from a categorylinks SQL dump file.");
    absl::ParseCommandLine(argc, argv);
    LOG(INFO) << "Reading category table from "
              << absl::GetFlag(FLAGS_category_dump) << "...";
    auto category_table =
        read_category_table(absl::GetFlag(FLAGS_category_dump));
    LOG(INFO) << "Done reading category table.";
    CategoryTreeIndex category_tree_index{absl::GetFlag(FLAGS_db_path),
                                          category_table};
    std::ifstream categorylinks_dump_stream{
        absl::GetFlag(FLAGS_categorylinks_dump)};
    LOG(INFO) << "Reading categorylinks table from "
              << absl::GetFlag(FLAGS_categorylinks_dump) << "...";
    read_categorylinks_table(category_tree_index,
                             absl::GetFlag(FLAGS_categorylinks_dump));
    LOG(INFO) << "Done reading categorylinks table. Saved to: "
              << absl::GetFlag(FLAGS_db_path);
    LOG(INFO) << "Starting to build weights...";
    category_tree_index.build_weights();
    LOG(INFO) << "Done building weights. Saved to: "
              << absl::GetFlag(FLAGS_db_path);
    return 0;
}