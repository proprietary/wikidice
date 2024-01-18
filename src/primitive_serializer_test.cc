#include <gtest/gtest.h>

#include "primitive_serializer.h"

TEST(PrimitiveSerializer, SerializeU64ToLowerEndian) {
    auto bytes = net_zelcon::wikidice::primitive_serializer::serialize_u64(
        0x1234567890ABCDEFULL);
    EXPECT_EQ(bytes[0], 0xEF);
    EXPECT_EQ(bytes[1], 0xCD);
    EXPECT_EQ(bytes[2], 0xAB);
    EXPECT_EQ(bytes[3], 0x90);
    EXPECT_EQ(bytes[4], 0x78);
    EXPECT_EQ(bytes[5], 0x56);
    EXPECT_EQ(bytes[6], 0x34);
    EXPECT_EQ(bytes[7], 0x12);
}

TEST(PrimitiveSerializer, EndToEnd) {
    for (uint64_t n = 0; n < 100'000'000; n++) {
        auto bytes =
            net_zelcon::wikidice::primitive_serializer::serialize_u64(n);
        auto res =
            net_zelcon::wikidice::primitive_serializer::deserialize_u64(bytes);
        ASSERT_EQ(n, res);
    }
}
