#include "category_tree_index.h"
#include "primitive_serializer.h"

#include <absl/log/check.h>
#include <absl/log/log.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iterator>
#include <queue>
#include <rocksdb/filter_policy.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>
#include <span>
#include <stdexcept>
#include <thread>
#include <unordered_set>

namespace net_zelcon::wikidice {

auto CategoryTreeIndexWriter::categorylinks_cf_options() const
    -> rocksdb::ColumnFamilyOptions {
    rocksdb::ColumnFamilyOptions cf_options;
    // add merge operator to merge CategoryLinkRecords
    cf_options.merge_operator.reset(new CategoryLinkRecordMergeOperator);
    // set write buffer size
    cf_options.write_buffer_size = 1 << 30; // 1GiB
    // optimizations for write-heavy workload
    cf_options.max_write_buffer_number =
        n_threads_; // allow more concurrent writes
                    // disable automatic compaction
    cf_options.disable_auto_compactions = true;
    return cf_options;
}

auto CategoryTreeIndex::categorylinks_cf_options() const
    -> rocksdb::ColumnFamilyOptions {
    rocksdb::ColumnFamilyOptions cf_options;
    // add merge operator to merge CategoryLinkRecords
    cf_options.merge_operator.reset(new CategoryLinkRecordMergeOperator);
    // enable compression
    cf_options.compression = rocksdb::kZSTD;
    cf_options.bottommost_compression = rocksdb::kZSTD;
    cf_options.compression_opts.level = 22;
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

auto CategoryTreeIndexWriter::db_options() const -> rocksdb::DBOptions {
    rocksdb::DBOptions options;
    options.create_if_missing = true;
    options.create_missing_column_families = true;
    options.error_if_exists = false;
    options.max_open_files = 1000;
    options.max_background_jobs = n_threads_;
    options.IncreaseParallelism(n_threads_);
    options.max_subcompactions = n_threads_;
    return options;
}

auto CategoryTreeIndex::db_options() const -> rocksdb::DBOptions {
    rocksdb::DBOptions options;
    options.create_if_missing = true;
    options.create_missing_column_families = true;
    options.error_if_exists = false;
    options.max_open_files = 1000;
    return options;
}

auto CategoryTreeIndex::category_id_to_name_cf_options() const
    -> rocksdb::ColumnFamilyOptions {
    rocksdb::ColumnFamilyOptions cf_options;
    cf_options.write_buffer_size = 64 * (1 << 20);
    // enable compression
    cf_options.compression = rocksdb::kZSTD;
    cf_options.bottommost_compression = rocksdb::kZSTD;
    cf_options.compression_opts.level = 22;
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
    auto db_options = this->db_options();
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

void CategoryTreeIndex::run_compaction() {
    LOG(INFO) << "Performing manual compaction on the RocksDB database...";
    rocksdb::CompactRangeOptions compact_options;
    compact_options.change_level = true;
    compact_options.target_level = 0;
    compact_options.bottommost_level_compaction =
        rocksdb::BottommostLevelCompaction::kForce;
    db_->CompactRange(compact_options, categorylinks_cf_, nullptr, nullptr);
}

auto CategoryTreeIndex::category_name_of(CategoryId category_id)
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

auto CategoryTreeIndexWriter::category_name_of(CategoryId category_id)
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
    if (status.IsNotFound()) {
        DLOG(INFO) << "category_name: " << category_name
                   << " not found in `categorylinks` column family";
        return std::nullopt;
    }
    DCHECK(status.ok()) << "Bad database state. Unable to retrieve category "
                           "link record for category: "
                        << category_name;
    LOG_IF(ERROR, !status.ok())
        << "Failed to get category_name: " << category_name
        << " status: " << status.ToString();
    // 2. Deserialize
    msgpack::zone zone;
    const auto o = msgpack::unpack(zone, value.data(), value.size());
    CHECK(!o.is_nil());
    return o.as<CategoryLinkRecord>();
}

CategoryTreeIndexWriter::CategoryTreeIndexWriter(
    const std::filesystem::path db_path,
    std::shared_ptr<CategoryTable> category_table,
    std::shared_ptr<WikiPageTable> wiki_page_table, uint32_t n_threads)
    : CategoryTreeIndex(db_path), category_table_{category_table},
      wiki_page_table_{wiki_page_table}, n_threads_{n_threads} {
    category_table_->for_each(
        [this](const CategoryRow &row) { import_category_row(row); });
    db_->CompactRange(rocksdb::CompactRangeOptions{}, category_id_to_name_cf_,
                      nullptr, nullptr);
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
    std::vector<CategoryLinksRow> rows{row};
    import_categorylinks_rows(rows);
}

void CategoryTreeIndexWriter::import_categorylinks_rows(
    const std::vector<CategoryLinksRow> &rows) {
    rocksdb::WriteBatch batch;
    for (const auto &row : rows) {
        switch (row.page_type) {
        case CategoryLinkType::FILE:
            // ignore; we are not indexing files
            break;
        case CategoryLinkType::PAGE:
            add_page(batch, row.category_name, row.page_id);
            break;
        case CategoryLinkType::SUBCAT: {
            auto subcategory_id = page_id_to_category_id(row.page_id);
            if (subcategory_id)
                add_subcategory(batch, row.category_name,
                                subcategory_id.value());
            break;
        }
        }
    }
    rocksdb::WriteOptions write_options;
    auto status = db_->Write(write_options, &batch);
    CHECK(status.ok()) << "Batch write of category link rows failed: "
                       << status.ToString();
    categorylinks_count_ += rows.size();
    if (categorylinks_count_ % 10'000'000 == 0)
        db_->CompactRange(rocksdb::CompactRangeOptions{}, categorylinks_cf_,
                          nullptr, nullptr);
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
    rocksdb::WriteBatch &batch, const std::string_view category_name,
    const CategoryId subcategory_id) {
    CategoryLinkRecord new_record{};
    new_record.subcategories_mut().push_back(subcategory_id);
    msgpack::sbuffer buf;
    msgpack::pack(buf, new_record);
    rocksdb::Slice value{buf.data(), buf.size()};
    auto status = batch.Merge(categorylinks_cf_, category_name, value);
    CHECK(status.ok()) << "Merge of record failed: " << status.ToString();
}

void CategoryTreeIndexWriter::add_page(rocksdb::WriteBatch &batch,
                                       const std::string_view category_name,
                                       const PageId page_id) {
    CategoryLinkRecord new_record{};
    new_record.pages_mut().push_back(page_id);
    msgpack::sbuffer buf;
    msgpack::pack(buf, new_record);
    rocksdb::Slice value{buf.data(), buf.size()};
    auto status = batch.Merge(categorylinks_cf_, category_name, value);
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
    -> std::optional<PageId> {
    const auto weight = lookup_weight(category_name);
    if (!weight) {
        return std::nullopt;
    }
    const auto picked =
        absl::Uniform<std::uint64_t>(random_generator, 0UL, weight.value());
    return at_index(category_name, picked);
}

auto CategoryTreeIndex::at_index(std::string_view category_name,
                                 std::uint64_t index) -> PageId {
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
    std::atomic<uint64_t> total_counter{0};
    std::vector<std::thread> threads;
    const uint64_t n_rows = count_rows();
    const uint64_t n_rows_per_thread = n_rows / n_threads_;
    LOG(INFO) << "Building weights for " << n_rows << " rows in " << n_threads_
              << " threads...";
    for (uint32_t i = 0; i < n_threads_; ++i) {
        threads.emplace_back(
            [this, i, n_rows_per_thread, read_options, &total_counter]() {
                auto begin_at = i * n_rows_per_thread;
                auto end_at = (i + 1) * n_rows_per_thread;
                std::unique_ptr<rocksdb::Iterator> it{
                    db_->NewIterator(read_options, categorylinks_cf_)};
                uint64_t internal_counter = 0;
                for (it->SeekToFirst();
                     it->Valid() && internal_counter < begin_at; it->Next())
                    internal_counter++;
                for (; it->Valid() &&
                       (i < (n_threads_ - 1) && internal_counter < end_at);
                     it->Next(), ++internal_counter) {
                    std::string_view category_name{it->key().data(),
                                                   it->key().size()};
                    const std::uint64_t weight = compute_weight(category_name);
                    set_weight(category_name, weight);
                    auto total_counter_result = total_counter.fetch_add(1);
                    if (total_counter_result % 1'000'000 == 0)
                        LOG(INFO) << "Built weights for "
                                  << total_counter_result << " entries so far";
                }
                CHECK(it->status().ok()) << it->status().ToString();
            });
    }
    for (auto &t : threads)
        t.join();
}

auto CategoryTreeIndexWriter::compute_weight(std::string_view category_name,
                                             float max_depth) -> std::uint64_t {
    uint64_t weight = 0;
    if (!get(category_name)) {
        LOG(WARNING) << "category name: " << std::quoted(category_name)
                     << "not found in `categorylinks` column family";
        return weight;
    }
    std::unordered_set<std::string> categories_visited;
    std::queue<std::string> categories_to_visit;
    categories_to_visit.emplace(category_name);
    float depth = 0.;
    while (!categories_to_visit.empty() && depth <= max_depth) {
        const auto top = categories_to_visit.front();
        categories_to_visit.pop();
        if (categories_visited.contains(top))
            continue; // this prevents a cycle
        categories_visited.insert(top);
        auto record = get(top);
        CHECK(record.has_value());
        if (record->weight() > 0) {
            // this category has already been visited, so we can skip the
            // expensive search because we have already computed its weight
            weight += record->weight();
        } else {
            // add number of pages in this category to "weight"
            weight += record->pages().size();
            // enqueue the subcategories of this category
            for (const auto &subcat : record->subcategories()) {
                auto subcat_name = category_name_of(subcat);
                CHECK(subcat_name.has_value());
                categories_to_visit.push(subcat_name.value());
            }
        }
        // increment depth
        depth += 1.0f;
    }
    return weight;
}

auto CategoryTreeIndexWriter::page_id_to_category_id(PageId page_id)
    -> std::optional<CategoryId> {
    auto page_row = wiki_page_table_->find(page_id);
    if (!page_row)
        return std::nullopt;
    auto category_row = category_table_->find(page_row->page_title);
    if (!category_row)
        return std::nullopt;
    return category_row->category_id;
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
    run_compaction();
}

CategoryTreeIndexReader::CategoryTreeIndexReader(
    const std::filesystem::path db_path)
    : CategoryTreeIndex(db_path) {}

auto CategoryTreeIndexReader::search_categories(
    std::string_view category_name_prefix) -> std::vector<std::string> {
    std::vector<std::string> autocompletions{};
    static constexpr size_t MAX_CATEGORY_NAME_PREFIX_LEN = 1'000;
    static constexpr size_t MAX_AUTOCOMPLETIONS = 100;
    if (category_name_prefix.length() > MAX_CATEGORY_NAME_PREFIX_LEN)
        return autocompletions;
    rocksdb::ReadOptions read_options;
    read_options.auto_prefix_mode = true;
    std::unique_ptr<rocksdb::Iterator> it{
        db_->NewIterator(read_options, categorylinks_cf_)};
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
    std::unique_ptr<rocksdb::Iterator> it{
        db_->NewIterator(read_options, categorylinks_cf_)};
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

auto CategoryTreeIndex::count_rows() -> uint64_t {
    uint64_t counter = 0;
    rocksdb::ReadOptions read_options;
    // optimize read options for sequential reads
    read_options.total_order_seek = true;
    read_options.readahead_size = 10 * (1 << 20); // 10MiB
    read_options.adaptive_readahead = true;
    std::unique_ptr<rocksdb::Iterator> it{
        db_->NewIterator(rocksdb::ReadOptions{}, categorylinks_cf_)};
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ++counter;
        if (counter % 1'000'000 == 0)
            LOG(INFO) << "Checked " << counter << " category link rows so far";
    }
    return counter;
}

auto CategoryTreeIndexWriter::count_rows() -> uint64_t {
    if (categorylinks_count_.load() > 0) {
        return categorylinks_count_.load();
    }
    return CategoryTreeIndex::count_rows();
}

} // namespace net_zelcon::wikidice
