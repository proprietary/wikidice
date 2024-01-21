#include "category_tree_index.h"
#include "entities/entities.h"
#include <absl/log/log.h>
#include <gtest/gtest.h>
#include <span>
#include <sstream>
#include <utility>

namespace net_zelcon::wikidice::test {

TEST(CategoryLinkRecordMergeOperatorTest, MergeTest) {
    CategoryLinkRecordMergeOperator mergeOperator;
    std::string_view category_name = "Physics";
    entities::CategoryLinkRecord existing;
    existing.pages_mut() = {1ULL, 2ULL, 3ULL};
    existing.subcategories_mut() = {99ULL, 98ULL, 97ULL};
    existing.weights_mut() = {{1, 2}, {2, 20}, {8, 30}, {9, 10001}};
    entities::CategoryLinkRecord new_record;
    new_record.pages_mut() = {4ULL, 5ULL, 6ULL};
    new_record.subcategories_mut() = {96ULL, 95ULL, 94ULL};
    new_record.weights_mut() = {{1, 3}, {2, 10}, {3, 30}, {4, 40}};
    const std::vector<uint64_t> expected_pages{1ULL, 2ULL, 3ULL,
                                               4ULL, 5ULL, 6ULL};
    const std::vector<uint64_t> expected_subcategories{94ULL, 95ULL, 96ULL,
                                                       97ULL, 98ULL, 99ULL};
    const std::vector<entities::CategoryWeight> expected_weight = {
        {1, 5}, {2, 30}, {3, 30}, {4, 40}, {8, 30}, {9, 10001}};
    std::vector<uint8_t> existing_record_bytes;
    entities::serialize(existing_record_bytes, existing);
    std::vector<uint8_t> new_record_bytes;
    entities::serialize(new_record_bytes, new_record);
    std::string output;
    rocksdb::Slice existing_value{
        reinterpret_cast<const char *>(existing_record_bytes.data()),
        existing_record_bytes.size()};
    rocksdb::Slice new_record_value{
        reinterpret_cast<const char *>(new_record_bytes.data()),
        new_record_bytes.size()};

    bool merge_result = mergeOperator.Merge(category_name, &existing_value,
                                            new_record_value, &output, nullptr);

    ASSERT_TRUE(merge_result);
    ASSERT_GT(output.size(), 0);
    entities::CategoryLinkRecord merged_record{};
    DLOG(INFO) << "about to deserialize merged record";
    entities::deserialize(
        merged_record,
        std::span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(output.data()), output.size()));
    DLOG(INFO) << "deserialized merged record";
    std::sort(merged_record.pages_mut().begin(),
              merged_record.pages_mut().end());
    ASSERT_EQ(expected_pages, merged_record.pages());
    std::sort(merged_record.subcategories_mut().begin(),
              merged_record.subcategories_mut().end());
    ASSERT_EQ(expected_subcategories, merged_record.subcategories());
    ASSERT_EQ(expected_weight, merged_record.weights());
}

TEST(CategoryLinkRecordMergeOperatorTest, NameTest) {
    CategoryLinkRecordMergeOperator mergeOperator;

    const char *name = mergeOperator.Name();

    EXPECT_STREQ(name, "CategoryLinkRecordMergeOperator");
}

} // namespace net_zelcon::wikidice::test