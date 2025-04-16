#include <gtest/gtest.h>
#include "ModularInt.hpp"
#include "NTT.hpp"
#include "NTTUtils.hpp"



TEST(NTT, NTTContextTest) {

    // We fix a small modulus for testing purposes
    constexpr uint32_t Q = 17;
    constexpr size_t N = 4;
    auto p = ModularInt(Q, N);

    constexpr uint32_t root = 13;  // Primitive 4-th root of unity mod 17
    
    auto ctx = NTTContext(Q, N, root);
    
    EXPECT_EQ( ctx.root, static_cast<uint32_t>(13));
    EXPECT_EQ( ctx.inv_root, static_cast<uint32_t>(4));  // Inverse of root mod Q (since 13^3 = 4 mod 17)
}

TEST(NTT, RoundTrip1) {
    // We fix a small modulus for testing purposes
    constexpr uint32_t Q = 17;
    constexpr size_t N = 4;
    auto p = ModularInt(Q, N);

    constexpr uint32_t root = 13;  // Primitive 4-th root of unity mod 17
    
    auto ctx = NTTContext(Q, N, root);
    
    p[0] = static_cast<uint16_t>(1);
    p[1] = static_cast<uint16_t>(2);
    p[2] = static_cast<uint16_t>(3);
    p[3] = static_cast<uint16_t>(4);
    forward_ntt(p, ctx); // This should become [10, 6, 15, 7]

    ASSERT_EQ( p[0], static_cast<uint16_t>(10));
    ASSERT_EQ( p[1], static_cast<uint16_t>(6));
    ASSERT_EQ( p[2], static_cast<uint16_t>(15));
    ASSERT_EQ( p[3], static_cast<uint16_t>(7));

    inverse_ntt(p, ctx); // This should return to [1, 2, 3, 4]
    
    ASSERT_EQ(p[0], static_cast<uint16_t>(1));
    ASSERT_EQ(p[1], static_cast<uint16_t>(2));
    ASSERT_EQ(p[2], static_cast<uint16_t>(3));
    ASSERT_EQ(p[3], static_cast<uint16_t>(4));
}




TEST(NTT, Roundtrip2) {

    /// ML-KEM-768 Parameters
    constexpr uint32_t Q = 3329;
    constexpr size_t N = 256;
    auto p = ModularInt(Q, N);

    for (size_t i = 0; i < N; ++i)
        p[i] = i % Q;

    auto ctx = NTTContext(Q, N, 17);
    // root is primitive N-th root

    forward_ntt(p, ctx);
    inverse_ntt(p, ctx);

    for (size_t i = 0; i < N; ++i) 
        ASSERT_EQ( p[i], static_cast< uint16_t >(i % Q)) << "Mismatch at index " << i;
    
    
}