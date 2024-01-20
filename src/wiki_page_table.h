#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <rocksdb/db.h>
#include <string>

#include "entities/entities.h"

namespace net_zelcon::wikidice {

class WikiPageTable {
  public:
    auto add_page(const entities::PageTableRow &page_row) -> void;
    auto find(const entities::PageId) -> std::optional<entities::PageTableRow>;
    explicit WikiPageTable(const std::filesystem::path &);
    ~WikiPageTable();
    WikiPageTable() = delete;
    WikiPageTable(const WikiPageTable &) = delete;
    WikiPageTable(WikiPageTable &&);
    auto operator=(WikiPageTable &) -> WikiPageTable & = delete;
    auto operator=(WikiPageTable &&) -> WikiPageTable &;

  private:
    rocksdb::DB *db_;
};

} // namespace net_zelcon::wikidice