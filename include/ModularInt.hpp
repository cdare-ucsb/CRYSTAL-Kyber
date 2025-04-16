#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <cmath>
#include <bit> 
#include <variant>
#include "ModularArith.hpp"




class ModularInt {
public:

    // using CoeffType = SmallestUInt_t<Q>;
    // using MA = ModArith<Q>;

    uint32_t Q;
    size_t N;
    size_t bitlen;
    std::vector<uint32_t> coeffs;

    

    ModularInt(uint64_t q, size_t n)
        : Q(q), N(n)
    {
        coeffs = std::vector<uint32_t>(N);
        bitlen = std::bit_width(q);
    }

    // Non-const accessor
    uint32_t& operator[](size_t idx) {
        return coeffs[idx];
    }

    // Const accessor
    const uint32_t& operator[](size_t idx) const {
        return coeffs[idx];
    }

    

    
    uint32_t sup_norm() const {
        uint32_t max_val = 0;

        for (auto c : coeffs) {
            uint32_t size = (c > Q / 2) ? (Q - c) : c;
            if (size > max_val) max_val = size;
        }

        return max_val;
    }

    size_t packed_bytes() const {
        return (bitlen * N + 7) / 8;
    }



    std::vector<uint8_t> pack() const {
        size_t out_size = packed_bytes();
        std::vector<uint8_t> out(out_size, 0);
        size_t bitpos = 0;

        for (size_t i = 0; i < N; ++i) {
            uint64_t val = coeffs[i] & ((1ULL << bitlen) - 1);

            size_t byte_index = bitpos / 8;
            int bit_offset = bitpos % 8;

            out[byte_index] |= (val << bit_offset) & 0xFF;
            if (bit_offset + bitlen > 8)
                out[byte_index + 1] |= (val >> (8 - bit_offset)) & 0xFF;
            if (bit_offset + bitlen > 16)
                out[byte_index + 2] |= (val >> (16 - bit_offset)) & 0xFF;

            bitpos += bitlen;
        }


        return out;
    }

    void unpack(const std::vector<uint8_t>& in) {
        size_t bitpos = 0;

        for (size_t i = 0; i < N; ++i) {
            size_t byte_index = bitpos / 8;
            int bit_offset = bitpos % 8;

            uint32_t val = (in[byte_index] >> bit_offset);
            if (bit_offset + bitlen > 8)
                val |= (in[byte_index + 1] << (8 - bit_offset));
            if (bit_offset + bitlen > 16)
                val |= (in[byte_index + 2] << (16 - bit_offset));

            coeffs[i] = val & ((1ULL << bitlen) - 1);
            bitpos += bitlen;
        }

    }

};
