#include <gtest/gtest.h>
#include <sstream>
#include <cstdint>

#include "entities.h"

namespace net_zelcon::wikidice::entities::test {

TEST(EntitiesTest, MergeCategoryWeightTest) {
  std::vector<CategoryWeight> lhs = {{1, 2}, {2, 20}, {8, 30}, {9, 10001}};
  std::vector<CategoryWeight> rhs = {{1, 5}, {2, 10}, {3, 30}, {4, 40}};

  merge(lhs, rhs);

  std::vector<CategoryWeight> expected = {{1, 7}, {2, 30}, {3, 30}, {4, 40}, {8, 30}, {9, 10001}};
  ASSERT_EQ(lhs, expected);
}

TEST(EntitiesTest, MsgpackSerialization) {
    CategoryLinkRecord record{};
    record.pages_mut() = {1ULL, 2ULL, 3ULL};
    record.subcategories_mut() = {99ULL, 98ULL, 97ULL};
    record.weights_mut() = {{1, 2}, {2, 20}, {8, 30}, {9, 10001}};
    std::stringstream buf;
    serialize(buf, record);
    ASSERT_GT(buf.str().size(), 0);
}

TEST(EntitiesTest, MsgpackSerializationWithVector) {
    CategoryLinkRecord record{};
    record.pages_mut() = {992ULL, 54ULL};
    record.subcategories_mut() = {};
    record.weights_mut() = {{2, 2}, {99, 6}, {100, 1200243}, {101, 11}};
    std::vector<uint8_t> bytes;
    serialize(bytes, record);
    ASSERT_GT(bytes.size(), 0);
    CategoryLinkRecord deserialized;
    deserialize(deserialized, bytes);
    ASSERT_EQ(deserialized, record);
    ASSERT_EQ(deserialized.pages(), record.pages());
    ASSERT_EQ(deserialized.subcategories(), record.subcategories());
    ASSERT_EQ(deserialized.weights(), record.weights());
}

TEST(EntitiesTest, MsgpackDeserialization) {
    CategoryLinkRecord record{};
    record.pages_mut() = {1ULL, 2ULL, 3ULL};
    record.subcategories_mut() = {99ULL, 98ULL, 97ULL};
    record.weights_mut() = {{1, 2}, {2, 20}, {8, 30}, {9, 10001}};
    std::stringstream buf;
    serialize(buf, record);
    std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(buf), {}};
    CategoryLinkRecord deserialized;
    deserialize(deserialized, bytes);
    ASSERT_EQ(deserialized, record);
    ASSERT_EQ(deserialized.pages(), record.pages());
    ASSERT_EQ(deserialized.subcategories(), record.subcategories());
    ASSERT_EQ(deserialized.weights(), record.weights());
}


} // namespace net_zelcon::wikidice::entities::test
