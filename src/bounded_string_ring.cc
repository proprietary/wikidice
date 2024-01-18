#include "bounded_string_ring.h"
#include <cstring>
#include <stdlib.h>
#include <string>
#include <string_view>

namespace net_zelcon::wikidice {

BoundedStringRing::BoundedStringRing(std::size_t size)
    : size_{size}, data_{new char[size]}, pos_{0ULL} {
    if (size <= 1) {
        delete[] data_;
        throw std::invalid_argument(
            "size must be greater than 1; otherwise just use a char");
    }
    std::memset(data_, '\0', size);
}

BoundedStringRing::~BoundedStringRing() noexcept { delete[] data_; }

void BoundedStringRing::push_back(char c) noexcept {
    pos_ %= size_;
    data_[pos_] = c;
    pos_++;
}

void BoundedStringRing::clear() noexcept { std::memset(data_, '\0', size_); }

bool BoundedStringRing::operator==(std::string_view other) const noexcept {
    if (other.size() != size_) {
        return false;
    }
    for (size_t i = 0; i < other.length(); ++i) {
        size_t this_index = (pos_ + i) % size_;
        if (data_[this_index] != other[i]) {
            return false;
        }
    }
    return true;
}

auto BoundedStringRing::string() const noexcept -> std::string {
    std::string output;
    output.reserve(size_);
    for (size_t i = 0; i < size_; ++i) {
        output.push_back(data_[(pos_ + i) % size_]);
    }
    return output;
}

auto BoundedStringRing::operator[](std::size_t i) const noexcept -> char {
    return data_[i % size_];
}

} // namespace net_zelcon::wikidice
