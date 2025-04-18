#include <gtest/gtest.h>
#include "ModularPoly.hpp"
#include "ModularInt.hpp"
#include "NTTUtils.hpp"



TEST(ModularPoly, ModularPolyCopy) {
    constexpr uint32_t Q = 3329;
    constexpr size_t N = 256;
    auto p = ModularPoly(N,Q);

    for (size_t i = 0; i < N; ++i)
        p[i] = ModularInt(i % Q, Q);

    auto p_copy = p;


    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(p[i], p_copy[i]) << "Mismatch at index " << i;
    }
}

TEST(ModularPolyPacking, RoundTrip) {

    /// ML-KEM-768 Parameters
    constexpr uint32_t Q = 3329;
    constexpr size_t N = 256;
    auto p = ModularPoly(N,Q);

    for (size_t i = 0; i < N; ++i)
        p[i] = ModularInt(i, Q);

    std::vector<uint8_t> buffer = p.pack();

    ModularPoly unpacked(N, Q);
    unpacked.unpack(buffer);

    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(p[i], unpacked[i]);
    }
}




TEST(ModularPoly, SupNormTest) {

    constexpr uint32_t Q = 3329;
    constexpr size_t N = 256;
    auto p = ModularPoly(N, Q);

    for (size_t i = 0; i < N; ++i)
        p[i] = ModularInt(i % Q, Q);

    uint16_t max_size = p.sup_norm();
    ASSERT_EQ(max_size, N - 1);
}
TEST(ModularPoly, SupNormWrapAroundTest) {

    constexpr uint32_t Q = 17;
    constexpr size_t N = 17;
    auto p = ModularPoly(N,Q);

    for (size_t i = 0; i < N; ++i)
        p[i] = ModularInt(i % Q, Q);

    uint16_t max_size = p.sup_norm();
    ASSERT_EQ(max_size, (Q-1) / 2);
}

TEST(ModularPoly, SupNormZeroTest) {

    constexpr uint32_t Q = 3329;
    constexpr size_t N = 256;
    auto p = ModularPoly(N,Q);

    for (size_t i = 0; i < N; ++i)
        p[i] = ModularInt(0, Q);

    uint16_t max_size = p.sup_norm();
    ASSERT_EQ(max_size, 0);
}

TEST(ModularPoly, NTTRoundTripTest) {
    constexpr uint32_t Q = 3329;
    constexpr size_t N = 256;
    constexpr uint32_t root = 17;

    auto p = ModularPoly(N, Q);

    for (size_t i = 0; i < N; ++i)
        p[i] = ModularInt(i % Q, Q);

    NTTContext ctx(Q, N, root);
    p.ctx = &ctx;

    // std::cout << "Before NTT: " << ctx.log_n << std::endl;
    p.NTT();
    p.INTT();

    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(p[i], ModularInt(i % Q, Q)) << "Mismatch at index " << i;
    }
}