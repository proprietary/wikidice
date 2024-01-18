#include "category_tree_index.h"
#include <gtest/gtest.h>
#include <msgpack.hpp>

namespace net_zelcon::wikidice::test {

TEST(CategoryTreeIndex, CategoryLinkRecordGettersAndSettersWork) {
    CategoryLinkRecord record1{};
    record1.pages_mut() = {1ULL};
    ASSERT_EQ(record1.pages(), std::vector<uint64_t>{1ULL});
    record1.subcategories_mut() = {99ULL};
    ASSERT_EQ(record1.subcategories(), std::vector<uint64_t>{99ULL});
    record1.weight_mut() = 10ULL;
    ASSERT_EQ(record1.weight(), 10ULL);
    CategoryLinkRecord record2{};
    record2.pages(std::vector<uint64_t>{1ULL});
    ASSERT_EQ(record2.pages(), std::vector<uint64_t>{1ULL});
    record2.subcategories(std::vector<uint64_t>{99ULL});
    ASSERT_EQ(record2.subcategories(), std::vector<uint64_t>{99ULL});
    record2.weight(10ULL);
    ASSERT_EQ(record2.weight(), 10ULL);
    ASSERT_EQ(record1, record2);
}

TEST(CategoryTreeIndex, TestSerialization) {
    CategoryLinkRecord record{};
    record.pages_mut() = {1ULL, 2ULL, 3ULL};
    record.subcategories_mut() = {99ULL, 98ULL, 97ULL};
    record.weight_mut() = 10ULL;
    msgpack::sbuffer buf;
    msgpack::pack(buf, record);
    ASSERT_GT(buf.size(), 0);
}

TEST(CategoryLinkRecordMergeOperatorTest, MergeTest) {
    CategoryLinkRecordMergeOperator mergeOperator;
    std::string_view category_name = "Physics";
    CategoryLinkRecord existing;
    existing.pages_mut() = {1ULL, 2ULL, 3ULL};
    existing.subcategories_mut() = {99ULL, 98ULL, 97ULL};
    existing.weight_mut() = 5ULL;
    CategoryLinkRecord new_record;
    new_record.pages_mut() = {4ULL, 5ULL, 6ULL};
    new_record.subcategories_mut() = {96ULL, 95ULL, 94ULL};
    new_record.weight_mut() = 10ULL;
    const std::vector<uint64_t> expected_pages{1ULL, 2ULL, 3ULL,
                                               4ULL, 5ULL, 6ULL};
    const std::vector<uint64_t> expected_subcategories{94ULL, 95ULL, 96ULL,
                                                       97ULL, 98ULL, 99ULL};
    const uint64_t expected_weight = 15ULL;
    msgpack::sbuffer existing_buf;
    msgpack::pack(existing_buf, existing);
    msgpack::sbuffer new_record_buf;
    msgpack::pack(new_record_buf, new_record);
    std::string output;
    rocksdb::Slice existing_value(existing_buf.data(), existing_buf.size());
    rocksdb::Slice new_record_value(new_record_buf.data(),
                                    new_record_buf.size());

    bool merge_result = mergeOperator.Merge(category_name, &existing_value,
                                            new_record_value, &output, nullptr);

    ASSERT_TRUE(merge_result);
    ASSERT_GT(output.size(), 0);
    msgpack::object_handle handle =
        msgpack::unpack(output.data(), output.size());
    CategoryLinkRecord merged_record = handle.get().as<CategoryLinkRecord>();
    std::sort(merged_record.pages_mut().begin(),
              merged_record.pages_mut().end());
    ASSERT_EQ(expected_pages, merged_record.pages());
    std::sort(merged_record.subcategories_mut().begin(),
              merged_record.subcategories_mut().end());
    ASSERT_EQ(expected_subcategories, merged_record.subcategories());
    ASSERT_EQ(expected_weight, merged_record.weight());
}

TEST(CategoryLinkRecordMergeOperatorTest, NameTest) {
    CategoryLinkRecordMergeOperator mergeOperator;

    const char *name = mergeOperator.Name();

    EXPECT_STREQ(name, "CategoryLinkRecordMergeOperator");
}

} // namespace net_zelcon::wikidice::test