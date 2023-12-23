#include "bounded_string_ring.h"

namespace net_zelcon::wikidice {

BoundedStringRing::BoundedStringRing(std::size_t size) : size_{size} {
    if (size == 0) {
        throw std::invalid_argument("size must be greater than 0");
    }
    iterator = new Node{'\0', nullptr};
    Node *prev = iterator;
    for (std::size_t i = 1; i < size; ++i) {
        Node *node = new Node{'\0', nullptr};
        prev->next = node;
        prev = node;
    }
    prev->next = iterator;
}

BoundedStringRing::~BoundedStringRing() noexcept {
    for (Node *n = iterator->next; n != iterator;) {
        Node *t = n;
        n = n->next;
        delete t;
    }
    delete iterator;
}

void BoundedStringRing::push_back(char c) noexcept {
    iterator->c = c;
    iterator = iterator->next;
}

bool BoundedStringRing::operator==(std::string_view other) const noexcept {
    if (other.length() != size_) {
        return false;
    }
    Node *cur = iterator;
    for (std::size_t i = 0; cur != nullptr && i < other.size(); ++i) {
        if (other[i] != cur->c) {
            return false;
        }
        cur = cur->next;
    }
    return true;
}

} // namespace net_zelcon::wikidice