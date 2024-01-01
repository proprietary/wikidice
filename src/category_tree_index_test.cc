#include <gtest/gtest.h>
#include "category_tree_index.h"
#include <msgpack.hpp>

namespace net_zelcon::wikidice::test {

TEST(CategoryTreeIndex, CategoryLinkRecordGettersAndSettersWork) {
    CategoryLinkRecord record1{};
    record1.pages_mut() = {1ULL};
    ASSERT_EQ(record1.pages(), std::vector<uint64_t>{1ULL});
    record1.subcategories_mut() = {"category1"};
    ASSERT_EQ(record1.subcategories(), std::vector<std::string>{"category1"});
    record1.weight_mut() = 10ULL;
    ASSERT_EQ(record1.weight(), 10ULL);
    CategoryLinkRecord record2{};
    record2.pages(std::vector<uint64_t>{1ULL});
    ASSERT_EQ(record2.pages(), std::vector<uint64_t>{1ULL});
    record2.subcategories(std::vector<std::string>{"category1"});
    ASSERT_EQ(record2.subcategories(), std::vector<std::string>{"category1"});
    record2.weight(10ULL);
    ASSERT_EQ(record2.weight(), 10ULL);
    ASSERT_EQ(record1, record2);
}

TEST(CategoryTreeIndex, TestSerialization) {
    CategoryLinkRecord record{};
    record.pages_mut() = {1ULL, 2ULL, 3ULL};
    record.subcategories_mut() = {"category1", "category2"};
    record.weight_mut() = 10ULL;
    msgpack::sbuffer buf;
    msgpack::pack(buf, record);
    ASSERT_GT(buf.size(), 0);
}

} // namespace net_zelcon::wikidice::test