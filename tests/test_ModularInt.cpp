#include <gtest/gtest.h>
#include "ModularPoly.hpp"
#include "ModularInt.hpp"

TEST(ModularInt, AddTest) {

    constexpr uint32_t Q = 3329;
    
    
    ModularInt a(100, Q);
    ModularInt b(200, Q);

    ASSERT_EQ(a + b, ModularInt(300, Q));
    ASSERT_EQ(a + ModularInt(300, Q), ModularInt(400, Q));
    ASSERT_EQ(a + ModularInt(3229, Q), ModularInt(0, Q));
    ASSERT_EQ(a + ModularInt(0, Q), a);
    ASSERT_EQ(a + ModularInt(3329, Q), a);
    
}

TEST(ModularInt, SubTest) {

    constexpr uint32_t Q = 3329;
    
    
    ModularInt a(100, Q);
    ModularInt b(200, Q);

    ASSERT_EQ(a - b, ModularInt(3329 - 100, Q));
    ASSERT_EQ(a - ModularInt(300, Q), ModularInt(3329 - 200, Q));
    ASSERT_EQ(a - ModularInt(3229, Q), ModularInt(200, Q));
    ASSERT_EQ(a - ModularInt(0, Q), a);
    ASSERT_EQ(a - ModularInt(3329, Q), a);
    
}

TEST(ModularInt, MulTest) {

    constexpr uint32_t Q = 3329;
    
    
    ModularInt a(100, Q);
    ModularInt b(200, Q);

    ASSERT_EQ(a * b, ModularInt(20000 % Q, Q));
    ASSERT_EQ(a * ModularInt(300, Q), ModularInt(30000 % Q, Q));
    ASSERT_EQ(a * ModularInt(0, Q), ModularInt(0, Q));
    ASSERT_EQ(a * ModularInt(3329, Q), ModularInt(0, Q));

    
}