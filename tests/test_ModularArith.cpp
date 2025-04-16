#include <gtest/gtest.h>
#include "ModularInt.hpp"
#include "ModularArith.hpp"



TEST(ModularArith, Add) {
    constexpr uint32_t Q = 3329;
    auto MA = ModularArith(Q);

    EXPECT_EQ( MA.add(static_cast<uint32_t>(1), static_cast<uint32_t>(2)), static_cast<uint32_t>(3));
    EXPECT_EQ( MA.add(static_cast<uint32_t>(1), Q - 1), static_cast<uint32_t>(0));
    EXPECT_EQ( MA.add(Q - 1, Q - 1), 2 * (Q - 1) % Q);
}

TEST(ModularArith, Sub) {

    constexpr uint32_t Q = 3329;
    auto MA = ModularArith(Q);

    EXPECT_EQ(MA.sub(static_cast<uint32_t>(3), static_cast<uint32_t>(2)), static_cast<uint32_t>(1));
    EXPECT_EQ(MA.sub(static_cast<uint32_t>(0), static_cast<uint32_t>(1)), Q - 1);
    EXPECT_EQ(MA.sub(Q - 1, Q - 1), 0);

    EXPECT_EQ(MA.sub(static_cast<uint32_t>(1), static_cast<uint32_t>(2)), Q - 1);
    EXPECT_EQ(MA.sub(static_cast<uint32_t>(5), static_cast<uint32_t>(10)), Q - 5);
}

TEST(ModularArith, Mul) {
    constexpr uint32_t Q = 3329;
    auto MA = ModularArith(Q);

    EXPECT_EQ( MA.mul(static_cast<uint32_t>(2), static_cast<uint32_t>(3)), static_cast<uint32_t>(6));
    EXPECT_EQ( MA.mul(static_cast<uint32_t>(2), Q - 1), (2 * (Q - 1)) % Q);
    EXPECT_EQ( MA.mul(Q - 1, Q - 1), ((Q - 1) * (Q - 1)) % Q);
    EXPECT_EQ( MA.mul(static_cast<uint32_t>(256), static_cast<uint32_t>(3316)), 1);

    EXPECT_EQ( MA.mul(static_cast<uint32_t>(256), static_cast<uint32_t>(0)), 0);
    EXPECT_EQ( MA.mul(static_cast<uint32_t>(0), static_cast<uint32_t>(256)), 0);
    
    EXPECT_EQ( MA.mul(static_cast<uint32_t>(24), static_cast<uint32_t>(367)), static_cast<uint32_t>(2150));
    EXPECT_EQ( MA.mul(static_cast<uint32_t>(367), static_cast<uint32_t>(24)), static_cast<uint32_t>(2150));
    
    EXPECT_EQ( MA.mul(static_cast<uint32_t>(24), static_cast<uint32_t>(3329)), 0);
}


TEST(ModularArith, Pow) {
    constexpr uint32_t Q = 3329;
    auto MA = ModularArith(Q);

    EXPECT_EQ( MA.pow(static_cast<uint32_t>(2), static_cast<uint32_t>(3)), 8);
    
    EXPECT_EQ( MA.pow(static_cast<uint32_t>(17), static_cast<uint32_t>(256)), 1);
}

TEST(ModularArith,Inv) {
    constexpr uint32_t Q = 3329;
    auto MA = ModularArith(Q);

    EXPECT_EQ( MA.inv(static_cast<uint32_t>(256)), 3316);
}

TEST(ModularArith, is_primitive_nth_root) {

    constexpr uint32_t Q = 17;
    auto MA = ModularArith(Q);

    EXPECT_TRUE(MA.is_primitive_nth_root(static_cast<uint32_t>(2), 8));

}