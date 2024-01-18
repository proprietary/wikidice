#include <gtest/gtest.h>

#include "bounded_string_ring.h"

using net_zelcon::wikidice::BoundedStringRing;

TEST(BoundedStringRing, TestBoundedStringRingSimple) {
    BoundedStringRing ring{5};
    ring.push_back('a');
    ring.push_back('b');
    ring.push_back('c');
    ring.push_back('d');
    ring.push_back('e');
    ASSERT_TRUE(ring == "abcde");
    ring.push_back('f');
    ASSERT_TRUE(ring == "bcdef");
    ring.push_back('q');
    ASSERT_TRUE(ring == "cdefq");
}

TEST(BoundedStringRing, TestBoundedStringRingOverflowWithSameCharacter) {
    BoundedStringRing ring{5};
    ring.push_back('a');
    ring.push_back('a');
    ring.push_back('a');
    ring.push_back('a');
    ring.push_back('a');
    ASSERT_TRUE(ring == "aaaaa");
    ring.push_back('a');
    ASSERT_TRUE(ring == "aaaaa");
    ring.push_back('b');
    ASSERT_TRUE(ring == "aaaab");
    ring.push_back('a');
    ASSERT_TRUE(ring == "aaaba");
}

TEST(BoundedStringRing, TestBoundedStringRingOverflow) {
    BoundedStringRing ring{5};
    ring.push_back('a');
    ring.push_back('b');
    ring.push_back('c');
    ring.push_back('d');
    ring.push_back('e');
    ASSERT_TRUE(ring == "abcde");
    ring.push_back('f');
    ASSERT_EQ("bcdef", ring.string());
    ring.push_back('g');
    ASSERT_TRUE(ring == "cdefg");
    ring.push_back('h');
    ASSERT_TRUE(ring == "defgh");
    ring.push_back('i');
    ASSERT_TRUE(ring == "efghi");
    ring.push_back('j');
    ASSERT_TRUE(ring == "fghij");
    ring.push_back('k');
    ASSERT_TRUE(ring == "ghijk");
    ring.push_back('l');
    ASSERT_TRUE(ring == "hijkl");
    ring.push_back('m');
    ASSERT_TRUE(ring == "ijklm");
}