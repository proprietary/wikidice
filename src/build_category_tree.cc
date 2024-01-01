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
#include <vector>

#include "build_category_tree.h"
#include "category_table.h"
#include "category_tree_index.h"
#include "sql_parser.h"

ABSL_FLAG(std::string, category_dump, "", "Path to the category SQL dump file");
ABSL_FLAG(std::string, categorylinks_dump, "",
          "Path to the categorylinks SQL dump file");
ABSL_FLAG(std::string, db_path, "", "Path to the database directory");
ABSL_FLAG(std::string, wikipedia_language_code, "en",
          "Wikipedia language code (e.g., \"en\", \"de\")");

namespace {

using namespace net_zelcon::wikidice;

auto read_category_table(const std::filesystem::path sqldump)
    -> std::shared_ptr<CategoryTable> {
    auto category_table = std::make_shared<CategoryTable>();
    uint64_t counter = 0;
    std::ifstream infile(sqldump);
    CategoryParser category_parser(infile);
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
                              const std::filesystem::path sqldump) -> void {
    std::ifstream infile{sqldump};
    CategoryLinksParser categorylinks_parser{infile};
    uint64_t counter = 0;
    while (auto row = categorylinks_parser.next()) {
        dst.import_categorylinks_row(row.value());
        if (++counter % 1'000'000 == 0)
            LOG(INFO) << fmt::format("Imported {:L} rows. Last imported row: "
                                     "page_id={} (a {}) → category_name={}",
                                     counter, row->page_id,
                                     to_string(row->page_type),
                                     row->category_name);
    }
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
    std::locale::global(std::locale("en_US.UTF-8"));
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
    read_categorylinks_table(category_tree_index,
                             absl::GetFlag(FLAGS_categorylinks_dump));
    LOG(INFO) << "Done reading categorylinks table. Saved to: "
              << db_destination_path.string();
    LOG(INFO) << "Starting to build weights...";
    category_tree_index.run_second_pass();
    LOG(INFO) << "Done building weights. Saved to: "
              << db_destination_path.string();
    return 0;
}