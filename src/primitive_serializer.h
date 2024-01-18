#pragma once

#include <absl/types/span.h>
#include <array>
#include <cstdint>

namespace net_zelcon::wikidice::primitive_serializer {

auto serialize_u64(uint64_t src) -> std::array<uint8_t, sizeof(uint64_t)>;

auto deserialize_u64(absl::Span<const uint8_t> src) -> uint64_t;

} // namespace net_zelcon::wikidice::primitive_serializer