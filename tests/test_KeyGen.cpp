#include <gtest/gtest.h>
#include "ModularPoly.hpp"
#include "ModularInt.hpp"
#include "KeyGen.hpp"
#include "NTTUtils.hpp"
#include "ModularMatrix.hpp"




TEST(KeyGen, GenerateModularPolyEntryTest) {

    constexpr uint32_t Q = 3329;
    constexpr size_t N = 256;
    std::array<uint8_t, 32> rho = {0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 1};
    ModularPoly poly(N, Q);

    generate_modular_poly_entry(poly, rho);

    ASSERT_EQ(poly.size(), N);
    ASSERT_LE(poly[0].val, Q - 1);
    ASSERT_GE(poly[0].val, 0);
}

TEST(KeyGen, GenerateModularMatrixTest) {
    constexpr uint32_t Q = 3329;
    constexpr size_t N = 256;
    std::array<uint8_t, 32> rho = {0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 1};
    size_t k = 4;                               
    ModularMatrix matrix(k, k, Q, N);

    generate_modular_matrix(matrix, rho);

    for (size_t i = 0; i < k; ++i) {
        for (size_t j = 0; j < k; ++j) {
            for (size_t l = 0; l < N; ++l) {
                ASSERT_LE(matrix(i,j)[l].val, Q - 1);
                ASSERT_GE(matrix(i,j)[l].val, 0);
            }
        }
    }
}