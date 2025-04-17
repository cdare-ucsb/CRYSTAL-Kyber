#pragma once

#include <cstdint>
#include <type_traits>
#include <random>
#include <vector>


#include "ModularInt.hpp"
#include "ModularPoly.hpp"
#include "NTTUtils.hpp"




struct ModularMatrix {
    size_t rows, cols;
    std::vector<ModularPoly> data;


    ModularMatrix() = default;

    ModularMatrix(size_t r, size_t c, uint32_t Q, size_t N)
        : rows(r), cols(c),
      data(r * c, ModularPoly(N, Q)) {}

      
    ModularPoly& operator()(size_t i, size_t j) {
        assert(i < rows && j < cols);
        return data[i * cols + j];
    }

    const ModularPoly& operator()(size_t i, size_t j) const {
        assert(i < rows && j < cols);
        return data[i * cols + j];
    }


    std::vector<ModularPoly> apply_transform(std::vector<ModularPoly>& input) {

        if (input.size() != this->cols) {
            throw std::invalid_argument("Input size does not match matrix dimensions.");
        }

        std::vector<ModularPoly> output;
        output.reserve(rows);

        for (size_t i = 0; i < rows; ++i) {
            ModularPoly acc(input[0].size(), input[0].Q);  // Q and N inferred from input
            for (size_t j = 0; j < cols; ++j) {
                acc = acc + (*this)(i, j) * input[j];
            }
            output.emplace_back(std::move(acc));
        }

        if (input[0].NTTed) {
            for (auto& entry : output) {
                entry.NTTed = true;
            }
        }

        return output;
    }

};

