#include "category_tree_index.h"

#include "cross_platform_endian.h"
#include <absl/log/check.h>
#include <absl/log/log.h>
#include <algorithm>
#include <array>
#include <fmt/core.h>
#include <iterator>
#include <queue>
#include <span>
#include <stdexcept>

namespace net_zelcon::wikidice {

namespace {

auto deserialize_pages(const rocksdb::Slice &data)
    -> std::vector<std::uint64_t> {
    msgpack::object_handle oh;
    std::vector<std::uint64_t> pages;
    msgpack::unpack(oh, data.data(), data.size());
    oh.get().convert(pages);
    return pages;
}

auto serialize_pages(msgpack::sbuffer &sbuf,
                     const std::vector<std::uint64_t> &pages) -> void {
    sbuf.clear();
    msgpack::pack(sbuf, pages);
}

auto deserialize_subcats(const rocksdb::Slice &data)
    -> std::vector<std::string> {
    msgpack::object_handle oh;
    std::vector<std::string> subcats;
    msgpack::unpack(oh, data.data(), data.size());
    oh.get().convert(subcats);
    return subcats;
}

auto serialize_subcats(msgpack::sbuffer &sbuf,
                       const std::vector<std::string> &subcats) -> void {
    sbuf.clear();
    msgpack::pack(sbuf, subcats);
}

auto deserialize_weight(const rocksdb::Slice &data) -> std::uint64_t {
    const uint64_t weight_le = *reinterpret_cast<const uint64_t *>(data.data());
    return le64toh(weight_le);
}

auto serialize_weight(const std::uint64_t weight)
    -> std::array<std::uint8_t, sizeof(std::uint64_t)> {
    auto weight_le = htole64(weight);
    std::array<std::uint8_t, sizeof(std::uint64_t)> result;
    std::copy(reinterpret_cast<std::uint8_t *>(&weight_le),
              reinterpret_cast<std::uint8_t *>(&weight_le) + sizeof(weight_le),
              std::begin(result));
    return result;
}

} // namespace

CategoryTreeIndex::CategoryTreeIndex(
    const std::filesystem::path db_path,
    std::shared_ptr<CategoryTable> category_table)
    : category_table_(category_table) {
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
    rocksdb::ColumnFamilyOptions cf_options;
    cf_options.compression = rocksdb::kLZ4Compression;
    column_families.emplace_back(rocksdb::kDefaultColumnFamilyName, cf_options);
    column_families.emplace_back("subcategories", cf_options);
    column_families.emplace_back("pages", cf_options);
    column_families.emplace_back("weight", cf_options);
    rocksdb::DBOptions db_options;
    db_options.create_if_missing = true;
    db_options.create_missing_column_families = true;
    db_options.error_if_exists = false;
    rocksdb::Status status =
        rocksdb::DB::Open(db_options, db_path.string(), column_families,
                          &column_family_handles_, &db_);
    if (!status.ok()) {
        LOG(FATAL) << fmt::format("failed to open db at {}: {}",
                                  db_path.string(), status.ToString());
    }
    CHECK(column_family_handles_.size() == 4);
    CHECK(column_family_handles_[1]->GetName() == "subcategories");
    subcategories_cf_ = column_family_handles_[1];
    CHECK(column_family_handles_[2]->GetName() == "pages");
    pages_cf_ = column_family_handles_[2];
    CHECK(column_family_handles_[3]->GetName() == "weight");
    weight_cf_ = column_family_handles_[3];
}

void CategoryTreeIndex::import_categorylinks_row(const CategoryLinksRow &row) {
    const auto category_row = category_table_->find(row.category_name);
    if (!category_row) {
        LOG(WARNING)
            << "category_name: " << row.category_name
            << " not found in `category` table, yet this category_name is a "
               "category according to the `categorylinks` table";
        return;
    }
    switch (row.page_type) {
    case CategoryLinkType::FILE:
        // ignore; we are not indexing files
        break;
    case CategoryLinkType::PAGE:
        add_page(row.category_name, row.page_id);
        break;
    case CategoryLinkType::SUBCAT:
        add_subcategory(row.category_name, category_row->category_name);
        break;
    }
}

void CategoryTreeIndex::add_subcategory(
    const std::string_view category_name,
    const std::string_view subcategory_name) {
    auto subcats = lookup_subcats(category_name);
    subcats.push_back(std::string{subcategory_name});
    set_subcategories(category_name, subcats);
}

void CategoryTreeIndex::add_page(const std::string_view category_name,
                                 const std::uint64_t page_id) {
    auto pages = lookup_pages(category_name);
    pages.push_back(page_id);
    set_category_members(category_name, pages);
}

auto CategoryTreeIndex::lookup_pages(std::string_view category_name)
    -> std::vector<std::uint64_t> {
    rocksdb::PinnableSlice value;
    const auto status =
        db_->Get(rocksdb::ReadOptions(), pages_cf_, category_name, &value);
    if (!status.ok()) {
        if (!status.IsNotFound())
            LOG(WARNING) << "failed to lookup pages for category_name: "
                         << category_name << " status: " << status.ToString();
        return {};
    }
    return deserialize_pages(value);
}

auto CategoryTreeIndex::lookup_subcats(std::string_view category_name)
    -> std::vector<std::string> {
    rocksdb::PinnableSlice value;
    rocksdb::ReadOptions read_options;
    const auto status =
        db_->Get(read_options, subcategories_cf_, category_name, &value);
    if (!status.ok()) {
        if (!status.IsNotFound())
            LOG(WARNING) << "failed to lookup subcategories for category_name: "
                         << category_name << " status: " << status.ToString();
        return {};
    }
    return deserialize_subcats(value);
}
auto CategoryTreeIndex::lookup_weight(std::string_view category_name)
    -> std::optional<std::uint64_t> {
    rocksdb::PinnableSlice value;
    rocksdb::ReadOptions read_options;
    const auto status =
        db_->Get(read_options, weight_cf_, category_name, &value);
    if (!status.ok()) {
        if (!status.IsNotFound())
            LOG(WARNING) << "failed to lookup weight for category_name: "
                         << category_name << " status: " << status.ToString();
        return std::nullopt;
    }
    return deserialize_weight(value);
}

void CategoryTreeIndex::set_category_members(
    const std::string_view category_name,
    const std::vector<std::uint64_t> &page_ids) {
    serialize_pages(sbuf_, page_ids);
    rocksdb::Slice value{sbuf_.data(), sbuf_.size()};
    const auto status =
        db_->Put(rocksdb::WriteOptions(), pages_cf_, category_name, value);
    CHECK(status.ok()) << status.ToString();
}

void CategoryTreeIndex::set_weight(const std::string_view category_name,
                                   const std::uint64_t weight) {
    const auto weight_le = serialize_weight(weight);
    rocksdb::Slice value{reinterpret_cast<const char *>(weight_le.data()),
                         weight_le.size()};
    rocksdb::WriteOptions write_options;
    const auto status =
        db_->Put(write_options, weight_cf_, category_name, value);
    CHECK(status.ok()) << status.ToString();
}

void CategoryTreeIndex::set_subcategories(
    const std::string_view category_name,
    const std::vector<std::string> &subcategories) {
    serialize_subcats(sbuf_, subcategories);
    rocksdb::Slice value{sbuf_.data(), sbuf_.size()};
    const auto status = db_->Put(rocksdb::WriteOptions(), subcategories_cf_,
                                 category_name, value);
    CHECK(status.ok()) << status.ToString();
}

auto CategoryTreeIndex::pick(std::string_view category_name,
                             absl::BitGenRef random_generator)
    -> std::optional<std::uint64_t> {
    const auto weight = lookup_weight(category_name);
    if (!weight.has_value()) {
        return std::nullopt;
    }
    const auto picked =
        absl::Uniform<std::uint64_t>(random_generator, 0UL, weight.value());
    return at_index(category_name, picked);
}

auto CategoryTreeIndex::at_index(std::string_view category_name,
                                 std::uint64_t index) -> std::uint64_t {
    const auto pages = lookup_pages(category_name);
    if (index < pages.size()) {
        return pages[index];
    }
    index -= pages.size();
    const auto subcats = lookup_subcats(category_name);
    for (const auto &subcat : subcats) {
        const auto weight = lookup_weight(subcat);
        if (!weight.has_value()) {
            LOG(WARNING) << "category_name: " << subcat
                         << " not found in `weight` column family";
            continue;
        }
        if (index < weight.value()) {
            return at_index(subcat, index);
        }
        index -= weight.value();
    }
    LOG(WARNING) << "index: " << index
                 << " out of range for category_name: " << category_name;
    return 0;
}

// Run second-pass to build the "weights" column family, which consists of the
// recursive sum of all the pages under a category.
void CategoryTreeIndex::build_weights() {
    rocksdb::ReadOptions read_options;
    read_options.pin_data = true;
    rocksdb::Iterator *it = db_->NewIterator(read_options, pages_cf_);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string_view category_name{it->key().data(), it->key().size()};
        const std::uint64_t weight = compute_weight(category_name);
        set_weight(category_name, weight);
    }
}

auto CategoryTreeIndex::compute_weight(std::string_view category_name)
    -> std::uint64_t {
    std::uint64_t weight = 0;
    const auto pages_in_category = lookup_pages(category_name);
    weight += pages_in_category.size();
    const auto subcats = lookup_subcats(category_name);
    for (const auto &subcat : subcats) {
        const auto subcat_weight = compute_weight(subcat);
        weight += subcat_weight;
    }
    return weight;
}

} // namespace net_zelcon::wikidice
