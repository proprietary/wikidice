#include "wiki_page_table.h"
#include "primitive_serializer.h"
#include <absl/log/check.h>
#include <absl/log/log.h>
#include <rocksdb/db.h>

namespace net_zelcon::wikidice {

WikiPageTable::WikiPageTable(const std::filesystem::path &db_path) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = true;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path.string(), &db_);
    CHECK(status.ok()) << status.ToString();
}

WikiPageTable::~WikiPageTable() { delete db_; }

WikiPageTable::WikiPageTable(WikiPageTable &&other) {
    db_ = other.db_;
    other.db_ = nullptr;
}

auto WikiPageTable::operator=(WikiPageTable &&other) -> WikiPageTable & {
    if (db_ != nullptr)
        delete db_;
    db_ = other.db_;
    other.db_ = nullptr;
    return *this;
}

auto WikiPageTable::add_page(const entities::PageTableRow &page_row) -> void {
    if (page_row.is_redirect || page_row.page_id == 0 ||
        page_row.page_title.empty())
        return;
    auto page_id = primitive_serializer::serialize_u64(page_row.page_id);
    rocksdb::Slice key(reinterpret_cast<const char *>(page_id.data()),
                       page_id.size());
    rocksdb::WriteOptions write_options;
    rocksdb::Status status = db_->Put(write_options, key, page_row.page_title);
    CHECK(status.ok()) << status.ToString();
}

auto WikiPageTable::find(const entities::PageId page_id)
    -> std::optional<entities::PageTableRow> {
    auto page_id_serialized = primitive_serializer::serialize_u64(page_id);
    rocksdb::Slice key(
        reinterpret_cast<const char *>(page_id_serialized.data()),
        page_id_serialized.size());
    std::string page_title;
    rocksdb::ReadOptions read_options;
    rocksdb::Status status = db_->Get(read_options, key, &page_title);
    if (status.IsNotFound()) {
        return std::nullopt;
    }
    CHECK(status.ok()) << status.ToString();
    return entities::PageTableRow{page_id, page_title, false};
}

} // namespace net_zelcon::wikidice