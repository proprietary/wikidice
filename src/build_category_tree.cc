#include <regex>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <absl/time/time.h>
#include <absl/time/clock.h>
#include <sqlite3.h>

#include "sql_parser.h"
#include "category_tree_index.h"

//auto read_sql(const std::filesystem::path sql_dump_filename, const std::filesystem::path outfile) -> void {
    //::sqlite_open(outfile.c_str());
//}

void import_sql(const std::filesystem::path sqldump) {
    spdlog::info("Importing SQL dump: {}", sqldump.string());
}

auto main(int, char**) -> int {
    absl::Time time = absl::Now();
    spdlog::info("Start time: {}", absl::FormatTime("%Y-%m-%d %H:%M:%S", time, absl::UTCTimeZone()));
    fmt::print("Start time: {}\n", absl::FormatTime("%Y-%m-%d %H:%M:%S", time, absl::UTCTimeZone()));
    fmt::print("Hello, {}\n", "world");
    return 0;
}