#pragma once

#include <cstdint>
#include <type_traits>
#include "SmallestUInt.hpp"
#include "ModularArith.hpp"
#include "NTTUtils.hpp"
#include <random>

#include <vector>


template <uint64_t Q, size_t N>
struct ModularMatrix {
    size_t rows, cols;
    std::vector<ModularInt<Q, N>> data;

    ModularMatrix(size_t r, size_t c) : rows(r), cols(c), data(r * c) {}

    ModularInt<Q, N>& operator()(size_t i, size_t j) {
        return data[i * cols + j];
    }

    const ModularInt<Q, N>& operator()(size_t i, size_t j) const {
        return data[i * cols + j];
    }

    void generate_from_hash(const NTTContext<Q, N>& ctx, seed_t seed) {

        std::default_random_engine generator(seed);
        std::uniform_int_distribution<int> distribution(0, Q);

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {

                for (size_t k = 0; k < N; ++k) {
                    data[i * cols + j][k] = distribution(generator);
                }
                forward_ntt(data[i * cols + j], ctx);

            }
        }
    }
};

