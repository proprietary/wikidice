#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <array>

namespace net_zelcon::wikidice {

/**
 * @class BoundedStringRing
 * @brief A ring buffer implementation for storing fixed-size strings.
 *
 * The BoundedStringRing class provides a ring buffer data structure for storing
 * fixed-size strings. It allows efficient insertion and retrieval of strings in
 * a circular manner.
 */
class BoundedStringRing {
private:
    const std::size_t size_;
    char* const data_;
    size_t pos_;

  public:
    /**
     * @brief A ring buffer implementation for storing fixed-size strings.
     *
     * The BoundedStringRing class provides a ring buffer data structure for
     * storing fixed-size strings. It allows efficient insertion and retrieval
     * of strings in a circular manner.
     *
     * @param size The length of the string to match
     * buffer.
     */
    explicit BoundedStringRing(std::size_t size);

    ~BoundedStringRing() noexcept;

    BoundedStringRing(const BoundedStringRing &) = delete;
    BoundedStringRing &operator=(const BoundedStringRing &) = delete;
    BoundedStringRing(BoundedStringRing &&) = delete;
    BoundedStringRing &operator=(BoundedStringRing &&) = delete;

    auto string() const noexcept -> std::string;

    auto operator[](std::size_t i) const noexcept -> char;

    /**
     * @brief Adds a character to the end of the string ring.
     *
     * @param c The character to be added.
     */
    void push_back(char c) noexcept;

    /**
     * @brief Overloaded equality operator for comparing a bounded_string_ring
     * object with a std::string_view.
     *
     * This operator compares the bounded_string_ring object with the provided
     * std::string_view and returns true if they are equal.
     *
     * @param other The std::string_view to compare with.
     * @return true if the bounded_string_ring object is equal to the provided
     * std::string_view, false otherwise.
     */
    bool operator==(std::string_view other) const noexcept;

    auto size() const noexcept -> std::size_t { return size_; }

    /**
     * @brief Clears the string ring. Resets the internal state without
     * deallocating any memory, so the object may be used again without new
     * allocations.
    */
    void clear() noexcept;
};

} // namespace net_zelcon::wikidice