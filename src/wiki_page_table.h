#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <rocksdb/db.h>
#include <string>

#include "category_link_type.h"

namespace net_zelcon::wikidice {

class WikiPageTable {
  public:
    auto add_page(const PageTableRow &page_row) -> void;
    auto find(const PageId) -> std::optional<PageTableRow>;
    explicit WikiPageTable(const std::filesystem::path &);
    ~WikiPageTable();
    WikiPageTable(const WikiPageTable &) = delete;
    WikiPageTable(WikiPageTable &&);
    auto operator=(WikiPageTable &) -> WikiPageTable & = delete;
    auto operator=(WikiPageTable &&) -> WikiPageTable &;

  private:
    rocksdb::DB *db_;
};

} // namespace net_zelcon::wikidice