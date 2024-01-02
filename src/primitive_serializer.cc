#include "primitive_serializer.h"
#include "cross_platform_endian.h"
#include <algorithm>
#include <cassert>

namespace net_zelcon::wikidice::primitive_serializer {

auto serialize_u64(uint64_t src) -> std::array<uint8_t, sizeof(uint64_t)> {
    static_assert(sizeof(uint64_t) == 8);
    std::array<uint8_t, sizeof(uint64_t)> bytes{};
    auto res = htole64(src);
    static_assert(sizeof(res) == sizeof(uint64_t));
    std::copy(reinterpret_cast<uint8_t *>(&res),
              reinterpret_cast<uint8_t *>(&res) + sizeof(res), bytes.begin());
    return bytes;
}

auto deserialize_u64(absl::Span<const uint8_t> src) -> uint64_t {
    static_assert(sizeof(uint64_t) == 8);
    assert(src.size() == sizeof(uint64_t));
    uint64_t res = 0ULL;
    std::copy(src.begin(), src.end(), reinterpret_cast<uint8_t *>(&res));
    static_assert(sizeof(res) == sizeof(uint64_t));
    return res;
}

} // namespace net_zelcon::wikidice::primitive_serializer