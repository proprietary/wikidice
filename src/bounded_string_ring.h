#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

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
    struct Node {
        char c;
        Node *next;
    };
    Node *iterator;
    const std::size_t size_;

  public:
    /**
     * @brief A ring buffer implementation for storing fixed-size strings.
     *
     * The BoundedStringRing class provides a ring buffer data structure for
     * storing fixed-size strings. It allows efficient insertion and retrieval
     * of strings in a circular manner.
     *
     * @param size The maximum number of strings that can be stored in the ring
     * buffer.
     */
    BoundedStringRing(std::size_t size);

    ~BoundedStringRing() noexcept;

    BoundedStringRing(const BoundedStringRing &) = delete;
    BoundedStringRing &operator=(const BoundedStringRing &) = delete;
    BoundedStringRing(BoundedStringRing &&) = delete;
    BoundedStringRing &operator=(BoundedStringRing &&) = delete;

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

    /// @brief Returns a string representation of the ring.
    /// @details Use this for debugging only. Prefer to use
    /// `operator==(std::string_view)`.
    /// @return the string in the buffer
    auto str() const noexcept -> std::string {
        std::string s;
        Node *cur = iterator;
        while (cur->next != iterator) {
            s.push_back(cur->c);
            cur = cur->next;
        }
        s.push_back(cur->c);
        return s;
    }
};

} // namespace net_zelcon::wikidice