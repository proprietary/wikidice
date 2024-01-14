#include "category_tree_index.h"
#include "primitive_serializer.h"

#include <absl/log/check.h>
#include <absl/log/log.h>
#include <algorithm>
#include <array>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iterator>
#include <queue>
#include <rocksdb/filter_policy.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>
#include <span>
#include <stdexcept>
#include <zstd.h>

namespace net_zelcon::wikidice {

auto CategoryTreeIndex::categorylinks_cf_options() const
    -> rocksdb::ColumnFamilyOptions {
    rocksdb::ColumnFamilyOptions cf_options;
    // add merge operator to merge CategoryLinkRecords
    cf_options.merge_operator.reset(new CategoryLinkRecordMergeOperator);
    // set write buffer size
    cf_options.write_buffer_size = 128 * (1 << 20);
    cf_options.max_write_buffer_number = 3;
    // enable compression
    cf_options.compression = rocksdb::kZSTD;
    cf_options.bottommost_compression = rocksdb::kZSTD;
    cf_options.compression_opts.level = ZSTD_maxCLevel();
    cf_options.compression_opts.max_dict_bytes = 8192;
    cf_options.compression_opts.zstd_max_train_bytes = 8192;
    // enable Bloom filter for efficient prefix seek
    rocksdb::BlockBasedTableOptions table_options;
    table_options.filter_policy.reset(
        rocksdb::NewRibbonFilterPolicy(BLOOM_FILTER_BITS_PER_KEY));
    cf_options.table_factory.reset(
        rocksdb::NewBlockBasedTableFactory(table_options));
    cf_options.prefix_extractor.reset(
        rocksdb::NewCappedPrefixTransform(PREFIX_CAP_LEN));
    return cf_options;
}

auto CategoryTreeIndex::category_id_to_name_cf_options() const
    -> rocksdb::ColumnFamilyOptions {
    rocksdb::ColumnFamilyOptions cf_options;
    cf_options.write_buffer_size = 64 * (1 << 20);
    // enable compression
    cf_options.compression = rocksdb::kZSTD;
    cf_options.bottommost_compression = rocksdb::kZSTD;
    cf_options.compression_opts.level = ZSTD_maxCLevel();
    cf_options.compression_opts.max_dict_bytes = 8192;
    cf_options.compression_opts.zstd_max_train_bytes = 8192;
    return cf_options;
}

CategoryTreeIndex::CategoryTreeIndex(const std::filesystem::path db_path) {
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
    // setup column families
    column_families.emplace_back(rocksdb::kDefaultColumnFamilyName,
                                 rocksdb::ColumnFamilyOptions{});
    column_families.emplace_back("categorylinks", categorylinks_cf_options());
    column_families.emplace_back("category_id_to_name",
                                 category_id_to_name_cf_options());
    // setup database
    rocksdb::DBOptions db_options;
    db_options.create_if_missing = true;
    db_options.create_missing_column_families = true;
    db_options.error_if_exists = false;
    db_options.max_open_files = 1000;
    rocksdb::Status status =
        rocksdb::DB::Open(db_options, db_path.string(), column_families,
                          &column_family_handles_, &db_);
    if (!status.ok()) {
        LOG(FATAL) << "Failed to open db at " << db_path.string() << ": "
                   << status.ToString();
    }
    CHECK_EQ(column_family_handles_.size(), 3ULL);
    CHECK_EQ(column_family_handles_[1]->GetName(), "categorylinks");
    categorylinks_cf_ = column_family_handles_[1];
    CHECK_EQ(column_family_handles_[2]->GetName(), "category_id_to_name");
    category_id_to_name_cf_ = column_family_handles_[2];
}

auto CategoryTreeIndex::category_name_of(uint64_t category_id)
    -> std::optional<std::string> {
    auto ser = primitive_serializer::serialize_u64(category_id);
    rocksdb::Slice key{reinterpret_cast<const char *>(ser.data()), ser.size()};
    rocksdb::PinnableSlice value;
    rocksdb::ReadOptions read_options;
    const auto status =
        db_->Get(read_options, category_id_to_name_cf_, key, &value);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            LOG(ERROR)
                << "Failed to find category name corresponding to category_id="
                << category_id
                << ". Column family `category_id_to_name` may not have been "
                   "populated yet, but should already be.";
            return std::nullopt;
        }
        LOG(FATAL) << "Failed to find category name of category_id="
                   << category_id << ": " << status.ToString();
    }
    return value.ToString();
}

auto CategoryTreeIndexWriter::category_name_of(uint64_t category_id)
    -> std::optional<std::string> {
    const auto category_row = category_table_->find(category_id);
    if (!category_row) {
        LOG(ERROR)
            << "Failed to find category name corresponding to category_id="
            << category_id
            << ". This is from an in-memory table derived from the SQL dump.";
        return std::nullopt;
    }
    return category_row->category_name;
}

auto CategoryTreeIndex::map_categories(absl::Span<const uint64_t> src)
    -> std::vector<std::string> {
    std::vector<std::string> dst;
    dst.reserve(src.size());
    for (auto cat_id : src) {
        auto category_name = category_name_of(cat_id);
        if (category_name)
            dst.emplace_back(category_name.value());
    }
    return dst;
}

auto CategoryTreeIndex::get(std::string_view category_name)
    -> std::optional<CategoryLinkRecord> {
    // 1. Retrieve from RocksDB database
    rocksdb::Slice key{category_name.data(), category_name.length()};
    rocksdb::PinnableSlice value;
    rocksdb::ReadOptions read_options;
    const auto status = db_->Get(read_options, categorylinks_cf_, key, &value);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            return std::nullopt;
        }
        // TODO: make this recoverable
        LOG(FATAL) << "Failed to get category_name: " << category_name
                   << " status: " << status.ToString();
    }
    // 2. Deserialize
    msgpack::zone zone;
    const auto o = msgpack::unpack(zone, value.data(), value.size());
    CHECK(!o.is_nil());
    return o.as<CategoryLinkRecord>();
}

CategoryTreeIndexWriter::CategoryTreeIndexWriter(
    const std::filesystem::path db_path,
    std::shared_ptr<CategoryTable> category_table)
    : CategoryTreeIndex(db_path), category_table_{category_table} {
    category_table_->for_each(
        [this](const CategoryRow &row) { import_category_row(row); });
}

auto CategoryTreeIndexWriter::set(std::string_view category_name,
                                  const CategoryLinkRecord &record) -> void {
    // 1. Serialize
    msgpack::sbuffer buf;
    msgpack::pack(buf, record);
    rocksdb::Slice value{buf.data(), buf.size()};
    rocksdb::Slice key{category_name.data(), category_name.size()};
    // 2. Add to RocksDB database
    rocksdb::WriteOptions write_options;
    auto status = db_->Put(write_options, categorylinks_cf_, key, value);
    CHECK(status.ok()) << "Write of record failed: " << status.ToString();
}

void CategoryTreeIndexWriter::import_categorylinks_row(
    const CategoryLinksRow &row) {
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

void CategoryTreeIndexWriter::import_category_row(const CategoryRow &row) {
    // 1. Serialize
    const auto category_id =
        primitive_serializer::serialize_u64(row.category_id);
    rocksdb::Slice key{reinterpret_cast<const char *>(category_id.data()),
                       category_id.size()};
    rocksdb::Slice value{row.category_name};
    // 2. Add to RocksDB database
    rocksdb::WriteOptions write_options;
    auto status = db_->Put(write_options, category_id_to_name_cf_, key, value);
    CHECK(status.ok()) << "Write of category row failed: " << status.ToString();
}

void CategoryTreeIndexWriter::add_subcategory(
    const std::string_view category_name,
    const std::string_view subcategory_name) {
    CategoryLinkRecord new_record{};
    const auto subcategory_details = category_table_->find(subcategory_name);
    if (!subcategory_details) {
        LOG(ERROR) << "In-memory table mapping category names to category IDs "
                      "is missing: "
                   << std::quoted(subcategory_name);
        return;
    }
    new_record.subcategories_mut().push_back(subcategory_details->category_id);
    msgpack::sbuffer buf;
    msgpack::pack(buf, new_record);
    rocksdb::Slice value{buf.data(), buf.size()};
    rocksdb::WriteOptions write_options;
    auto status =
        db_->Merge(write_options, categorylinks_cf_, category_name, value);
    CHECK(status.ok()) << "Merge of record failed: " << status.ToString();
}

void CategoryTreeIndexWriter::add_page(const std::string_view category_name,
                                       const std::uint64_t page_id) {
    CategoryLinkRecord new_record{};
    new_record.pages_mut().push_back(page_id);
    msgpack::sbuffer buf;
    msgpack::pack(buf, new_record);
    rocksdb::Slice value{buf.data(), buf.size()};
    rocksdb::WriteOptions write_options;
    auto status =
        db_->Merge(write_options, categorylinks_cf_, category_name, value);
    CHECK(status.ok()) << "Merge of record failed: " << status.ToString();
}

auto CategoryTreeIndex::lookup_pages(std::string_view category_name)
    -> std::vector<std::uint64_t> {
    auto record = get(category_name);
    if (record)
        return record->pages();
    else
        return {};
}

auto CategoryTreeIndex::lookup_subcats(std::string_view category_name)
    -> std::vector<std::string> {
    auto record = get(category_name);
    if (record)
        return map_categories(record->subcategories());
    else
        return {};
}

auto CategoryTreeIndex::lookup_weight(std::string_view category_name)
    -> std::optional<std::uint64_t> {
    auto record = get(category_name);
    if (record)
        return record->weight();
    else
        return std::nullopt;
}

void CategoryTreeIndexWriter::set_weight(const std::string_view category_name,
                                         const std::uint64_t weight) {
    CategoryLinkRecord new_record{};
    new_record.weight(weight);
    msgpack::sbuffer buf;
    msgpack::pack(buf, new_record);
    rocksdb::Slice value{buf.data(), buf.size()};
    rocksdb::WriteOptions write_options;
    auto status =
        db_->Merge(write_options, categorylinks_cf_, category_name, value);
    CHECK(status.ok()) << "Merge of record failed: " << status.ToString();
}

auto CategoryTreeIndexReader::pick(std::string_view category_name,
                                   absl::BitGenRef random_generator)
    -> std::optional<std::uint64_t> {
    const auto weight = lookup_weight(category_name);
    if (!weight) {
        return std::nullopt;
    }
    const auto picked =
        absl::Uniform<std::uint64_t>(random_generator, 0UL, weight.value());
    return at_index(category_name, picked);
}

auto CategoryTreeIndex::at_index(std::string_view category_name,
                                 std::uint64_t index) -> std::uint64_t {
    auto record = get(category_name);
    if (!record)
        LOG(WARNING) << "category_name: " << category_name
                     << " not found in `categorylinks` column family";
    const auto &pages = record->pages();
    if (index < pages.size()) {
        return pages[index];
    }
    index -= pages.size();
    for (const auto &subcat : map_categories(record->subcategories())) {
        const auto weight = lookup_weight(subcat);
        if (!weight.has_value()) {
            LOG(WARNING) << "category_name: " << subcat
                         << " not found in `categorylinks` column family";
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
void CategoryTreeIndexWriter::build_weights() {
    rocksdb::ReadOptions read_options;
    read_options.adaptive_readahead = true;
    read_options.total_order_seek = true; // ignore prefix Bloom filter in read
    rocksdb::Iterator *it = db_->NewIterator(read_options, categorylinks_cf_);
    uint64_t counter = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string_view category_name{it->key().data(), it->key().size()};
        const std::uint64_t weight = compute_weight(category_name);
        set_weight(category_name, weight);
        if (++counter % 1'000'000)
            LOG(INFO) << "Built weights for " << counter << " entries so far";
    }
    CHECK(it->status().ok()) << it->status().ToString();
}

auto CategoryTreeIndexWriter::compute_weight(
    std::string_view category_name, float max_depth) -> std::uint64_t {
    uint64_t weight = 0;
    if (!get(category_name)) {
        LOG(WARNING) << "category name: " << std::quoted(category_name)
                     << "not found in `categorylinks` column family";
        return weight;
    }
    std::vector<std::string> categories_visited;
    std::queue<std::string> categories_to_visit;
    categories_to_visit.emplace(category_name);
    float depth = 0.;
    while (!categories_to_visit.empty() && depth <= max_depth) {
        const auto top = categories_to_visit.front();
        categories_to_visit.pop();
        if (std::ranges::find(categories_visited, top) !=
            categories_visited.end())
            continue; // this prevents a cycle
        categories_visited.push_back(top);
        // add number of pages in this category to "weight"
        weight += lookup_pages(top).size();
        // enqueue the subcategories of this category
        for (const auto &subcat : lookup_subcats(top))
            categories_to_visit.push(subcat);
        // increment depth
        depth += 1.0f;
    }
    return weight;
}

auto CategoryTreeIndexWriter::run_second_pass() -> void {
    // build the "weights" (the number of leaf nodes (pages) under a category)
    LOG(INFO) << "Building the weights (aka the number of leaf nodes (pages) "
                 "under a category)...";
    build_weights();
    // flush the write buffer
    LOG(INFO) << "Flushing RocksDB write buffer...";
    rocksdb::FlushOptions flush_options;
    flush_options.wait = true;
    db_->Flush(flush_options);
    // perform manual compaction
    LOG(INFO) << "Performing manual compaction on the RocksDB database...";
    rocksdb::CompactRangeOptions compact_options;
    compact_options.change_level = true;
    compact_options.target_level = 0;
    compact_options.bottommost_level_compaction =
        rocksdb::BottommostLevelCompaction::kForce;
    db_->CompactRange(compact_options, categorylinks_cf_, nullptr, nullptr);
}

auto CategoryTreeIndexReader::search_categories(
    std::string_view category_name_prefix) -> std::vector<std::string> {
    std::vector<std::string> autocompletions{};
    static constexpr size_t MAX_CATEGORY_NAME_PREFIX_LEN = 1'000;
    static constexpr size_t MAX_AUTOCOMPLETIONS = 100;
    if (category_name_prefix.length() > MAX_CATEGORY_NAME_PREFIX_LEN)
        return autocompletions;
    rocksdb::ReadOptions read_options;
    read_options.auto_prefix_mode = true;
    rocksdb::Iterator *it = db_->NewIterator(read_options, categorylinks_cf_);
    for (it->Seek(category_name_prefix);
         it->Valid() && it->key().starts_with(category_name_prefix) &&
         autocompletions.size() < MAX_AUTOCOMPLETIONS;
         it->Next()) {
        autocompletions.emplace_back(it->key().data(), it->key().size());
    }
    return autocompletions;
}

auto CategoryTreeIndexReader::for_each(
    std::function<bool(std::string_view, const CategoryLinkRecord &)> consumer)
    -> void {
    rocksdb::ReadOptions read_options;
    read_options.total_order_seek = true;
    rocksdb::Iterator *it = db_->NewIterator(read_options, categorylinks_cf_);
    CategoryLinkRecord record{};
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string_view category_name{it->key().data(), it->key().size()};
        msgpack::zone zone;
        msgpack::unpack(zone, it->value().data(), it->value().size())
            .convert(record);
        const bool keep_going = consumer(category_name, record);
        if (!keep_going)
            break;
    }
}

bool CategoryLinkRecordMergeOperator::Merge(
    const rocksdb::Slice &, const rocksdb::Slice *existing_value,
    const rocksdb::Slice &value, std::string *new_value,
    rocksdb::Logger *) const {
    if (existing_value) {
        msgpack::zone zone;
        auto existing_record = msgpack::unpack(zone, existing_value->data(),
                                               existing_value->size())
                                   .as<CategoryLinkRecord>();
        zone.clear();
        auto new_record = msgpack::unpack(zone, value.data(), value.size())
                              .as<CategoryLinkRecord>();
        new_record += existing_record;
        msgpack::sbuffer sbuf;
        msgpack::pack(sbuf, new_record);
        new_value->assign(sbuf.data(), sbuf.size());
    } else {
        new_value->assign(value.data(), value.size());
    }
    return true;
}

auto CategoryTreeIndex::to_string(const CategoryLinkRecord &record)
    -> std::string {
    auto subcat_names = map_categories(record.subcategories());
    return fmt::format("CategoryLinkRecord: pages = {}, subcategories = {}, "
                       "subcategories (nums) = {}, weight = {}",
                       record.pages(), subcat_names, record.subcategories(),
                       record.weight());
}

} // namespace net_zelcon::wikidice
