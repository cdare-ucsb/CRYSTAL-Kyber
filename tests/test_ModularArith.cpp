#include <gtest/gtest.h>
#include "ModularInt.hpp"
#include "SmallestUInt.hpp"
#include "ModularArith.hpp"




TEST(ModularArith, Add) {
    constexpr uint64_t Q = 3329;
    using MA = ModArith<Q>;
    using T = SmallestUInt_t<Q>;

    EXPECT_EQ(MA::add(1, 2), 3);
    EXPECT_EQ(MA::add(1, Q - 1), 0);
    EXPECT_EQ(MA::add(Q - 1, Q - 1), 2 * (Q - 1) % Q);
}

TEST(ModularArith, Sub) {

    constexpr uint64_t Q = 3329;
    using MA = ModArith<Q>;
    using T = SmallestUInt_t<Q>;

    EXPECT_EQ(MA::sub(3, 2), 1);
    EXPECT_EQ(MA::sub(0, 1), Q - 1);
    EXPECT_EQ(MA::sub(Q - 1, Q - 1), 0);

    EXPECT_EQ(MA::sub(1, 2), Q - 1);
    EXPECT_EQ(MA::sub(5, 10), Q - 5);
}

TEST(ModularArith, Mul) {
    constexpr uint64_t Q = 3329;
    using MA = ModArith<Q>;
    using T = SmallestUInt_t<Q>;

    EXPECT_EQ(MA::mul(2, 3), 6);
    EXPECT_EQ(MA::mul(2, Q - 1), (2 * (Q - 1)) % Q);
    EXPECT_EQ(MA::mul(Q - 1, Q - 1), ((Q - 1) * (Q - 1)) % Q);
    EXPECT_EQ(MA::mul(256, 3316), 1);

    EXPECT_EQ(MA::mul(256, 0), 0);
    EXPECT_EQ(MA::mul(0, 256), 0);
    
    EXPECT_EQ(MA::mul(24, 367), 2150);
    EXPECT_EQ(MA::mul(367, 24), 2150);
    
    EXPECT_EQ(MA::mul(24, 3329), 0);
}


TEST(ModularArith, Pow) {
    constexpr uint64_t Q = 3329;
    using MA = ModArith<Q>;
    using T = SmallestUInt_t<Q>;

    EXPECT_EQ(MA::pow(2, 3), 8);
    
    EXPECT_EQ(MA::pow(17, 256), 1);
}

TEST(ModularArith,Inv) {
    EXPECT_EQ(ModArith<3329>::inv(256), 3316);
}