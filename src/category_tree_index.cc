#include "category_tree_index.h"
#include "entities/entities.h"
#include "primitive_serializer.h"

#include <absl/log/check.h>
#include <absl/log/log.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iterator>
#include <queue>
#include <rocksdb/filter_policy.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>
#include <span>
#include <stack>
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
    options.error_if_exists = true;
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

auto CategoryTreeIndex::category_name_of(entities::CategoryId category_id)
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

auto CategoryTreeIndexWriter::category_name_of(entities::CategoryId category_id)
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
    -> std::optional<entities::CategoryLinkRecord> {
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
    std::span<const uint8_t> data{
        reinterpret_cast<const uint8_t *>(value.data()), value.size()};
    entities::CategoryLinkRecord output;
    entities::deserialize(output, data);
    return output;
}

CategoryTreeIndexWriter::CategoryTreeIndexWriter(
    const std::filesystem::path db_path,
    std::shared_ptr<CategoryTable> category_table,
    std::shared_ptr<WikiPageTable> wiki_page_table, uint32_t n_threads)
    : CategoryTreeIndex(db_path), category_table_{category_table},
      wiki_page_table_{wiki_page_table}, n_threads_{n_threads} {
    category_table_->for_each(
        [this](const entities::CategoryRow &row) { import_category_row(row); });
    db_->CompactRange(rocksdb::CompactRangeOptions{}, category_id_to_name_cf_,
                      nullptr, nullptr);
}

auto CategoryTreeIndexWriter::set(std::string_view category_name,
                                  const entities::CategoryLinkRecord &record)
    -> void {
    // 1. Serialize
    std::vector<uint8_t> buf;
    entities::serialize(buf, record);
    rocksdb::Slice value{reinterpret_cast<const char *>(buf.data()),
                         buf.size()};
    // 2. Add to RocksDB database
    rocksdb::WriteOptions write_options;
    auto status =
        db_->Put(write_options, categorylinks_cf_, category_name, value);
    CHECK(status.ok()) << "Write of record failed: " << status.ToString();
}

void CategoryTreeIndexWriter::import_categorylinks_row(
    const entities::CategoryLinksRow &row) {
    std::vector<entities::CategoryLinksRow> rows{row};
    import_categorylinks_rows(rows);
}

void CategoryTreeIndexWriter::import_categorylinks_rows(
    const std::vector<const entities::CategoryLinksRow *> &rows) {
    rocksdb::WriteBatch batch;
    for (const auto &row : rows) {
        switch (row->page_type) {
        case entities::CategoryLinkType::FILE:
            // ignore; we are not indexing files
            break;
        case entities::CategoryLinkType::PAGE:
            add_page(batch, row->category_name, row->page_id);
            break;
        case entities::CategoryLinkType::SUBCAT: {
            auto subcategory_id = page_id_to_category_id(row->page_id);
            if (subcategory_id)
                add_subcategory(batch, row->category_name,
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
}

void CategoryTreeIndexWriter::import_categorylinks_rows(
    const std::vector<entities::CategoryLinksRow> &rows) {
    std::vector<const entities::CategoryLinksRow *> ptrs;
    ptrs.reserve(rows.size());
    for (size_t i = 0; i < rows.size(); ++i) {
        const entities::CategoryLinksRow *row = &rows[i];
        ptrs.push_back(row);
    }
    import_categorylinks_rows(ptrs);
}

void CategoryTreeIndexWriter::import_category_row(
    const entities::CategoryRow &row) {
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
    const entities::CategoryId subcategory_id) {
    entities::CategoryLinkRecord new_record{};
    new_record.subcategories_mut().push_back(subcategory_id);
    std::vector<uint8_t> buf;
    entities::serialize(buf, new_record);
    rocksdb::Slice value{reinterpret_cast<const char *>(buf.data()),
                         buf.size()};
    auto status = batch.Merge(categorylinks_cf_, category_name, value);
    CHECK(status.ok()) << "Merge of record failed: " << status.ToString();
}

void CategoryTreeIndexWriter::add_page(rocksdb::WriteBatch &batch,
                                       const std::string_view category_name,
                                       const entities::PageId page_id) {
    entities::CategoryLinkRecord new_record{};
    new_record.pages_mut().push_back(page_id);
    std::vector<uint8_t> buf;
    entities::serialize(buf, new_record);
    rocksdb::Slice value{reinterpret_cast<const char *>(buf.data()),
                         buf.size()};
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

auto CategoryTreeIndex::at_index(std::string_view category_name,
                                 std::uint64_t index,
                                 uint8_t depth) -> entities::PageId {
    auto record = get(category_name);
    if (!record) {
        LOG(WARNING) << "category_name: " << category_name
                     << " not found in `categorylinks` column family";
        return 0;
    }
    const auto &pages = record->pages();
    if (index < pages.size()) {
        return pages[index];
    }
    index -= pages.size();
    for (const auto &subcat : map_categories(record->subcategories())) {
        const auto cat_details = get(subcat);
        if (!cat_details) {
            LOG(WARNING) << "category_name: " << subcat
                         << " not found in `categorylinks` column family";
            continue;
        }
        auto weight_at_depth = cat_details->weight_at_depth(depth);
        if (weight_at_depth == 0)
            weight_at_depth = compute_weight(category_name, depth);
        if (index < weight_at_depth) {
            return at_index(subcat, index, depth);
        }
        index -= weight_at_depth;
    }
    LOG(WARNING) << "index: " << index
                 << " out of range for category_name: " << category_name;
    return 0;
}

auto CategoryTreeIndex::resolve_index_with_derivation(
    std::string_view category_name, std::uint64_t index,
    uint8_t depth) -> std::tuple<entities::PageId, std::vector<std::string>> {
    std::vector<std::string> derivation;
    std::stack<std::string> stack;
    stack.emplace(category_name);
    while (!stack.empty()) {
        auto top = stack.top();
        stack.pop();
        auto record = get(top);
        if (!record) {
            LOG(WARNING) << "category_name: " << top
                         << " not found in `categorylinks` column family";
            continue;
        }
        derivation.push_back(top);
        if (index < record->pages().size()) {
            return std::make_tuple(record->pages()[index], derivation);
        }
        index -= record->pages().size();
        for (const auto &subcat : map_categories(record->subcategories())) {
            const auto cat_details = get(subcat);
            if (!cat_details) {
                LOG(WARNING) << "category_name: " << subcat
                             << " not found in `categorylinks` column family";
                continue;
            }
            auto weight_at_depth = cat_details->weight_at_depth(depth);
            if (weight_at_depth == 0)
                weight_at_depth = compute_weight(category_name, depth);
            if (index < weight_at_depth) {
                stack.emplace(subcat);
                break;
            }
            index -= weight_at_depth;
        }
    }
    LOG(WARNING) << "index: " << index
                 << " out of range for category_name: " << category_name;
    return std::make_tuple(0, derivation);
}

auto CategoryTreeIndexWriter::build_weights(
    std::string_view category_name, const uint8_t depth_begin,
    const uint8_t depth_end) -> std::vector<entities::CategoryWeight> {
    std::vector<entities::CategoryWeight> weights;
    weights.reserve(depth_end - depth_begin + 1);
    auto depth = depth_begin;
    static constexpr size_t kStopIfRepeatedNTimes = 5;
    for (; depth <= depth_end; ++depth) {
        entities::CategoryWeight weight;
        weight.weight = compute_weight(category_name, depth);
        weight.depth = depth;
        if (depth > (depth_begin + kStopIfRepeatedNTimes) &&
            std::all_of(weights.rbegin(),
                        weights.rbegin() + kStopIfRepeatedNTimes,
                        [&weights](const auto &w) {
                            return w.weight == weights.back().weight;
                        }))
            break;
        weights.push_back(weight);
    }
    return weights;
}

auto CategoryTreeIndex::compute_weight(std::string_view category_name,
                                       uint8_t max_depth) -> std::uint64_t {
    uint64_t weight = 0;
    if (!get(category_name)) {
        LOG(WARNING) << "category name: " << std::quoted(category_name)
                     << " not found in `categorylinks` column family";
        return weight;
    }
    std::unordered_set<std::string> categories_visited;
    std::queue<std::string> categories_to_visit;
    categories_to_visit.emplace(category_name);
    uint8_t depth = 0;
    while (!categories_to_visit.empty() && depth <= max_depth) {
        const auto top = categories_to_visit.front();
        categories_to_visit.pop();
        if (categories_visited.contains(top))
            continue; // this prevents a cycle
        categories_visited.insert(top);
        auto record = get(top);
        if (!record.has_value())
            // This means the category has no pages or subcategories. It usually
            // means all the pages in this category are files. We don't care
            // about files.
            continue;
        // add number of pages in this category to "weight"
        weight += record->pages().size();
        // enqueue the subcategories of this category
        for (const auto &subcat : record->subcategories()) {
            auto subcat_name = category_name_of(subcat);
            CHECK(subcat_name.has_value());
            categories_to_visit.push(subcat_name.value());
        }
        // increment depth
        depth += 1;
    }
    return weight;
}

auto CategoryTreeIndexWriter::page_id_to_category_id(entities::PageId page_id)
    -> std::optional<entities::CategoryId> {
    auto page_row = wiki_page_table_->find(page_id);
    if (!page_row)
        return std::nullopt;
    auto category_row = category_table_->find(page_row->page_title);
    if (!category_row)
        return std::nullopt;
    return category_row->category_id;
}

auto CategoryTreeIndexWriter::run_second_pass() -> void {

    LOG(INFO) << "Pruning the dangling subcategories (subcategories that do "
                 "not have real _article_ (not file) pages underneath them)...";
    for_each([this](std::string_view category_name,
                    entities::CategoryLinkRecord &record) {
        uint64_t counter = 0;
        const auto before_len = record.subcategories().size();
        prune_dangling_subcategories(record);
        if (record.subcategories().size() != before_len) {
            set(category_name, record);
        }
        counter++;
        LOG_IF(INFO, counter % 100'000 == 0)
            << "Pruned dangling subcategories for " << counter
            << " categories so far";
    });
    std::atomic<uint64_t> counter{0};
    // build the "weights" (the number of leaf nodes (pages) under a category)
    LOG(INFO) << "Building the weights (aka the number of leaf nodes (pages) "
                 "under a category)...";
    // set up RocksDB writer thread
    std::atomic<bool> done{false};
    boost::lockfree::queue<
        std::pair<std::string, entities::CategoryLinkRecord> *,
        boost::lockfree::capacity<10'000>>
        queue{};
    std::thread writer_thread{[this, &queue, &done]() {
        std::pair<std::string, entities::CategoryLinkRecord> *entry;
        while (!done) {
            while (queue.pop(entry)) {
                set(entry->first, entry->second);
                delete entry;
            }
        }
    }};
    parallel_for_each(
        [this, &counter, &queue](std::string_view category_name,
                                 entities::CategoryLinkRecord &record) {
            const auto weights =
                build_weights(category_name, DEPTH_BEGIN, DEPTH_END);
            record.weights_mut().assign(weights.begin(), weights.end());
            std::pair<std::string, entities::CategoryLinkRecord> *entry =
                new std::pair<std::string, entities::CategoryLinkRecord>{
                    std::string{category_name}, std::move(record)};
            while (!queue.push(entry)) {
                std::this_thread::yield();
            }
            const auto counter_result = counter.fetch_add(1);
            LOG_IF(INFO, counter_result % 100'000 == 0)
                << "Built weights for " << counter_result
                << " categories so far";
        },
        n_threads_ - 1);
    done = true;
    writer_thread.join();
    // flush the write buffer
    LOG(INFO) << "Flushing RocksDB write buffer...";
    rocksdb::FlushOptions flush_options;
    flush_options.wait = true;
    db_->Flush(flush_options);
    // perform manual compaction
    db_->CompactRange(rocksdb::CompactRangeOptions{}, categorylinks_cf_,
                      nullptr, nullptr);
}

CategoryTreeIndexReader::CategoryTreeIndexReader(
    const std::filesystem::path db_path)
    : CategoryTreeIndex(db_path) {}

auto CategoryTreeIndexReader::search_categories(
    std::string_view category_name_prefix,
    size_t requested_count) -> std::vector<std::string> {
    std::vector<std::string> autocompletions{};
    static constexpr size_t MAX_CATEGORY_NAME_PREFIX_LEN = 1'000;
    static constexpr size_t MAX_AUTOCOMPLETIONS = 100;
    requested_count = std::min(requested_count, MAX_AUTOCOMPLETIONS);
    if (category_name_prefix.length() > MAX_CATEGORY_NAME_PREFIX_LEN)
        return autocompletions;
    rocksdb::ReadOptions read_options;
    read_options.auto_prefix_mode = true;
    std::unique_ptr<rocksdb::Iterator> it{
        db_->NewIterator(read_options, categorylinks_cf_)};
    for (it->Seek(category_name_prefix);
         it->Valid() && it->key().starts_with(category_name_prefix) &&
         autocompletions.size() < requested_count;
         it->Next()) {
        autocompletions.emplace_back(it->key().data(), it->key().size());
    }
    return autocompletions;
}

auto CategoryTreeIndexReader::for_each(
    std::function<bool(std::string_view, const entities::CategoryLinkRecord &)>
        consumer) -> void {
    rocksdb::ReadOptions read_options;
    read_options.total_order_seek = true;
    std::unique_ptr<rocksdb::Iterator> it{
        db_->NewIterator(read_options, categorylinks_cf_)};
    entities::CategoryLinkRecord record{};
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string_view category_name{it->key().data(), it->key().size()};
        entities::deserialize(
            record, std::span<const uint8_t>{
                        reinterpret_cast<const uint8_t *>(it->value().data()),
                        it->value().size()});
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
        entities::CategoryLinkRecord existing_record;
        entities::deserialize(
            existing_record,
            std::span<const uint8_t>{
                reinterpret_cast<const uint8_t *>(existing_value->data()),
                existing_value->size()});
        entities::CategoryLinkRecord new_record;
        entities::deserialize(
            new_record,
            std::span<const uint8_t>{
                reinterpret_cast<const uint8_t *>(value.data()), value.size()});
        new_record += std::move(existing_record);
        std::stringstream sbuf;
        entities::serialize(sbuf, new_record);
        new_value->assign(std::move(sbuf).str());
    } else {
        new_value->assign(value.data(), value.size());
    }
    return true;
}

auto CategoryTreeIndex::to_string(const entities::CategoryLinkRecord &record)
    -> std::string {
    auto subcat_names = map_categories(record.subcategories());
    return fmt::format(
        "CategoryLinkRecord: pages = {}, subcategories = {}, "
        "subcategories (nums) = {}, weights = {}",
        record.pages(), subcat_names, record.subcategories(),
        entities::to_string(record.weights().begin(), record.weights().end()));
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

auto CategoryTreeIndexReader::pick_at_depth(std::string_view category_name,
                                            uint8_t depth, absl::BitGenRef gen)
    -> std::optional<entities::PageId> {
    const auto record = get(category_name);
    if (!record)
        return std::nullopt;
    const auto picked =
        absl::Uniform<std::uint64_t>(gen, 0UL, record->weight_at_depth(depth));
    return at_index(category_name, picked, depth);
}

auto CategoryTreeIndexReader::pick_at_depth_and_show_derivation(
    std::string_view category_name, uint8_t depth, absl::BitGenRef gen)
    -> std::optional<std::tuple<entities::PageId, std::vector<std::string>>> {
    const auto record = get(category_name);
    if (!record)
        return std::nullopt;
    const auto picked =
        absl::Uniform<std::uint64_t>(gen, 0UL, record->weight_at_depth(depth));
    return resolve_index_with_derivation(category_name, picked, depth);
}

auto CategoryTreeIndexWriter::set_weight(std::string_view category_name,
                                         const entities::CategoryWeight &weight)
    -> void {
    auto record = get(category_name);
    if (!record)
        return;
    std::sort(
        record->weights_mut().begin(), record->weights_mut().end(),
        [](const auto &lhs, const auto &rhs) { return lhs.depth < rhs.depth; });
    auto insertion_point = std::lower_bound(
        record->weights_mut().begin(), record->weights_mut().end(), weight,
        [](const auto &lhs, const auto &rhs) { return lhs.depth < rhs.depth; });
    if (insertion_point == record->weights_mut().end() ||
        insertion_point->depth != weight.depth)
        record->weights_mut().insert(insertion_point, weight);
    else if (insertion_point->depth == weight.depth)
        *insertion_point = weight;
    DCHECK(std::is_sorted(record->weights().begin(), record->weights().end(),
                          [](const auto &lhs, const auto &rhs) {
                              return lhs.depth < rhs.depth;
                          }));
}

auto CategoryTreeIndexWriter::prune_dangling_subcategories(
    entities::CategoryLinkRecord &record) -> void {
    // This removes the subcategories that only have "file" pages in them. This
    // is because we don't care about files. No one wants to see a file when
    // they ask for a random article.
    for (auto subcat_it = std::begin(record.subcategories_mut());
         subcat_it != std::end(record.subcategories_mut());) {
        const auto subcategory_name = category_name_of(*subcat_it);
        if (!subcategory_name || !get(*subcategory_name)) {
            subcat_it = record.subcategories_mut().erase(subcat_it);
        } else {
            subcat_it++;
        }
    }
}

} // namespace net_zelcon::wikidice
