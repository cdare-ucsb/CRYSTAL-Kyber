#include <gtest/gtest.h>
#include "Poly.hpp"

constexpr uint64_t Q = 3329;
constexpr size_t N = 256;
using MyPoly = Poly<Q, N>;

TEST(PolyPacking, RoundTrip) {
    MyPoly p;
    for (size_t i = 0; i < N; ++i)
        p[i] = i % Q;

    std::array<uint8_t, MyPoly::PACKED_BYTES> buf;
    p.pack(buf);

    MyPoly unpacked;
    unpacked.unpack(buf);

    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(p[i], unpacked[i]);
    }
}
