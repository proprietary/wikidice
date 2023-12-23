#include "category_tree_index.h"

#include <stdexcept>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <absl/log/log.h>
#include <absl/log/check.h>

namespace net_zelcon::wikidice {

CategoryTreeIndex::CategoryTreeIndex(const std::filesystem::path db_path) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.compression = rocksdb::kLZ4Compression;
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
    rocksdb::ColumnFamilyOptions cf_options;
    cf_options.compression = rocksdb::kLZ4Compression;
    column_families.emplace_back(rocksdb::kDefaultColumnFamilyName,
                                 cf_options);
    column_families.emplace_back("subcategories",
                                cf_options);
    column_families.emplace_back("pages", cf_options);
    column_families.emplace_back("weight", cf_options);
    rocksdb::DBOptions db_options;
    db_options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(db_options, db_path.string(), column_families, &column_family_handles_, &db_);    
    if (!status.ok()) {
        spdlog::error("failed to open db at {}: {}", db_path.string(), status.ToString());
        throw std::runtime_error("failed to open db");
    }
    CHECK(column_family_handles_.size() == 4);
    CHECK(column_family_handles_[1]->GetName() == "subcategories");
    subcategories_cf_ = column_family_handles_[1];
    CHECK(column_family_handles_[2]->GetName() == "pages");
    pages_cf_ = column_family_handles_[2];
    CHECK(column_family_handles_[3]->GetName() == "weight");
    weight_cf_ = column_family_handles_[3];
}

void CategoryTreeIndex::import_row(const std::uint64_t page_id,
                                   const std::string_view category_name,
                                   const CategoryLinkType category_link_type) {
    auto category_row = category_table_.find(page_id);
    if (category_row == category_table_.end()) {
        LOG(WARNING) << "page_id: " << page_id << " not found in `category` table, yet this page_id is a subcategory according to the `categorylinks` table";
        return;
    }
    rocksdb::WriteBatch batch;
    switch (category_link_type) {
    case CategoryLinkType::SUBCAT: {
        add_subcategory(batch, category_name, category_row->second.category_name);
        break;
    }
    case CategoryLinkType::PAGE: {
        add_page(batch, category_name, page_id);
        add_to_weight(batch, category_name, 1);
        const auto ancestral_categories = ancestral_categories_of(category_row->second.category_id);
        for (const auto& ancestral_category : ancestral_categories) {
            add_to_weight(batch, ancestral_category, 1);
        }
        break;
    }
    case CategoryLinkType::FILE:
        // ignore; we are not indexing files
        break;
    }
    rocksdb::WriteOptions write_options;
    const auto status = db_->Write(write_options, &batch);
    CHECK(status.ok()) << status.ToString();
}

void CategoryTreeIndex::perform_second_pass() {
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions(), subcategories_cf_);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        const auto& key = it->key();
        const auto& value = it->value();
        const auto category_name = std::string_view(key.data(), key.size());
        const auto subcategory_name = std::string_view(value.data(), value.size());
        rocksdb::WriteBatch batch;
        add_subcategory(batch, category_name, subcategory_name);
        add_to_weight(batch, category_name, lookup_weight(category_name));
        const auto status = db_->Write(rocksdb::WriteOptions(), &batch);
        CHECK(status.ok()) << status.ToString();
    }
}

void CategoryTreeIndex::setup_column_families() {
    rocksdb::ColumnFamilyHandle *subcategories_cf;
    auto subcats_options = rocksdb::ColumnFamilyOptions();
    db_->CreateColumnFamily(subcats_options, "subcategories",
                            &subcategories_cf);
    rocksdb::ColumnFamilyHandle *pages_cf;
    db_->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), "pages", &pages_cf);
    rocksdb::ColumnFamilyHandle *weight_cf;
    db_->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), "weight",
                            &weight_cf);
}

auto CategoryTreeIndex::ancestral_categories_of(const std::uint64_t page_id) -> std::vector<std::string> {
    LOG(FATAL) << "not implemented: " << __PRETTY_FUNCTION__ << " page_id: " << page_id;
    throw std::runtime_error("not implemented");
}

void CategoryTreeIndex::add_subcategory(rocksdb::WriteBatch&, const std::string_view category_name, const std::string_view subcategory_name) {
    LOG(FATAL) << "not implemented: " << __PRETTY_FUNCTION__ << " category_name: " << category_name << " subcategory_name: " << subcategory_name;
}

void CategoryTreeIndex::add_page(rocksdb::WriteBatch&, const std::string_view category_name, const std::uint64_t page_id) {
    LOG(FATAL) << "not implemented: " << __PRETTY_FUNCTION__ << " category_name: " << category_name << " page_id: " << page_id;
}

void CategoryTreeIndex::add_to_weight(rocksdb::WriteBatch&, const std::string_view category_name, const std::uint64_t weight_to_add) {
    LOG(FATAL) << "not implemented: " << __PRETTY_FUNCTION__ << " category_name: " << category_name << " weight_to_add: " << weight_to_add;
}

auto CategoryTreeIndex::lookup_pages(std::string_view category_name) -> std::vector<std::uint64_t> {
    LOG(FATAL) << "not implemented: " << __PRETTY_FUNCTION__ << " category_name: " << category_name;
}
auto CategoryTreeIndex::lookup_subcats(std::string_view category_name) -> std::vector<std::string> {
    LOG(FATAL) << "not implemented: " << __PRETTY_FUNCTION__ << " category_name: " << category_name;
}
auto CategoryTreeIndex::lookup_weight(std::string_view category_name) -> std::uint64_t {
    LOG(FATAL) << "not implemented: " << __PRETTY_FUNCTION__ << " category_name: " << category_name;
}


} // namespace net_zelcon::wikidice
