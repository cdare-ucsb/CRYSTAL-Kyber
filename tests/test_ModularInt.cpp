#include <gtest/gtest.h>
#include "ModularInt.hpp"
#include "SmallestUInt.hpp"


TEST(CopyCheck, ModularIntCopy) {
    constexpr uint64_t Q = 3329;
    constexpr size_t N = 256;
    using Poly = ModularInt<Q, N>;

    Poly p;
    for (size_t i = 0; i < N; ++i)
        p[i] = i % Q;

    Poly p_copy = p;

    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(p[i], p_copy[i]) << "Mismatch at index " << i;
    }
}

TEST(ModularIntPacking, RoundTrip) {

    /// ML-KEM-768 Parameters
    constexpr uint64_t Q = 3329;
    constexpr size_t N = 256;
    using KEMPoly = ModularInt<Q, N>;

    KEMPoly p;
    for (size_t i = 0; i < N; ++i)
        p[i] = i % Q;

    std::array<uint8_t, KEMPoly::PACKED_BYTES> buf;
    p.pack(buf);

    KEMPoly unpacked;
    unpacked.unpack(buf);

    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(p[i], unpacked[i]);
    }
}

TEST(ModularIntStorage, StorageSizeSmall) {

    constexpr uint64_t Q = 17;
    constexpr size_t N = 3;
    using SmallPoly = ModularInt<Q, N>;

    static_assert(std::is_same_v<typename SmallPoly::CoeffType, uint8_t>,
        "Expected uint8_t coefficient storage for Q <= 256");

    // Optional: Confirm actual size of storage
    EXPECT_EQ(sizeof(SmallPoly::CoeffType), 1);
    EXPECT_EQ(sizeof(SmallPoly), sizeof(uint8_t) * N);
}

TEST(ModularIntStorage, StorageSizeMedium) {

    constexpr uint64_t Q = 16383;
    constexpr size_t N = 80;
    using SmallPoly = ModularInt<Q, N>;

    static_assert(std::is_same_v<typename SmallPoly::CoeffType, uint16_t>,
        "Expected uint16_t coefficient storage for Q <= 65536");

    // Optional: Confirm actual size of storage
    EXPECT_EQ(sizeof(SmallPoly::CoeffType), 2);
    EXPECT_EQ(sizeof(SmallPoly), sizeof(uint16_t) * N);
}

TEST(ModularIntStorage, StorageSizeLarge) {

    constexpr uint64_t Q = 131071;
    constexpr size_t N = 320;
    using SmallPoly = ModularInt<Q, N>;

    static_assert(std::is_same_v<typename SmallPoly::CoeffType, uint32_t>,
        "Expected uint16_t coefficient storage for Q <= 4294967296");

    // Optional: Confirm actual size of storage
    EXPECT_EQ(sizeof(SmallPoly::CoeffType), 4);
    EXPECT_EQ(sizeof(SmallPoly), sizeof(uint32_t) * N);
}


TEST(ModularIntSupNorm, SupNormTest) {

    constexpr uint64_t Q = 3329;
    constexpr size_t N = 256;
    using KEMPoly = ModularInt<Q, N>;

    KEMPoly p;
    for (size_t i = 0; i < N; ++i)
        p[i] = i % Q;

    uint16_t max_size = p.sup_norm();
    ASSERT_EQ(max_size, N - 1);
}
TEST(ModularIntSupNorm, SupNormNegativeTest) {

    constexpr uint64_t Q = 17;
    constexpr size_t N = 17;
    using KEMPoly = ModularInt<Q, N>;

    KEMPoly p;
    for (size_t i = 0; i < N; ++i)
        p[i] = i % Q;

    uint16_t max_size = p.sup_norm();
    ASSERT_EQ(max_size, (Q-1) / 2);
}