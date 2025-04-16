#pragma once

#include <cstdint>
#include <type_traits>
#include <random>
#include <vector>


#include "ModularArith.hpp"
#include "ModularInt.hpp"
#include "NTTUtils.hpp"
#include "NTT.hpp"



struct ModularMatrix {
    size_t rows, cols;
    std::vector<ModularInt> data;

    ModularMatrix() = default;

    ModularMatrix(size_t r, size_t c, const NTTContext& ctx)
    : rows(r), cols(c),
      data(r * c, ModularInt(ctx.Q, ctx.N)) {}

      
    ModularInt& operator()(size_t i, size_t j) {
        return data[i * cols + j];
    }

    const ModularInt& operator()(size_t i, size_t j) const {
        return data[i * cols + j];
    }

    void generate_from_hash(const NTTContext& ctx, unsigned long seed) {

        std::default_random_engine generator(seed);
        std::uniform_int_distribution<uint32_t> distribution(0,  ctx.Q);

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                for (size_t k = 0; k < ctx.N; ++k) {
                    data[i * cols + j][k] = distribution(generator);
                }
                forward_ntt(data[i * cols + j], ctx);

            }
        }
    }
};

