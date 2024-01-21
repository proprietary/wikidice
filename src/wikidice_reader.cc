#include "wikidice_query.h"
#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/log/check.h>
#include <absl/log/log.h>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

ABSL_FLAG(std::string, db_path, "", "Path to database directory");
ABSL_FLAG(std::string, category_name, "", "Name of category to query");
ABSL_FLAG(int, depth, 0,
          "Number of nested categories to search to find a random article");

namespace net_zelcon::wikidice {} // namespace net_zelcon::wikidice

int main(int argc, char *argv[]) {
    using net_zelcon::wikidice::Session;
    absl::ParseCommandLine(argc, argv);
    const std::string db_path = absl::GetFlag(FLAGS_db_path);
    Session session{std::string_view{db_path}};
    const std::string category_name = absl::GetFlag(FLAGS_category_name);
    CHECK_LT(absl::GetFlag(FLAGS_depth), std::numeric_limits<uint8_t>::max())
        << "Depth must be less than 256";
    const uint8_t depth = static_cast<uint8_t>(absl::GetFlag(FLAGS_depth));
    auto [article_id, success] =
        session.pick_random_article(category_name, depth);
    LOG_IF(ERROR, !success) << "No article found in category " << category_name;
    LOG(INFO) << "Article ID: " << article_id;
    std::cout << "https://en.wikipedia.org/wiki?curid=" << article_id
              << std::endl;
    return EXIT_SUCCESS;
}
