/*!
 * \file Poly.hpp
 * \brief Template class for storing and packing polynomials over Z_q[x]/(x^N + 1)
 *
 * Provides support for compile-time selected modulus Q and degree N,
 * with bit-level packing and coefficient type deduction.
 */
#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <cmath>
#include <bit> 
#include "SmallestUInt.hpp"



/*!
 * \class Poly
 * \brief Represents a polynomial in Z_q[x] / (x^N + 1)
 *
 * \tparam Q The modulus q. Coefficients are in Z_q.
 * \tparam N The number of coefficients (polynomials of degree < N)
 *
 * Internally, coefficients are stored using the smallest unsigned integer
 * type that fits the modulus q (e.g., uint8_t, uint16_t, uint32_t).
 * Supports bit-efficient packing and unpacking into byte arrays.
 */
template <uint64_t Q, size_t N>
class Poly {
public:
    using CoeffType = SmallestUInt_t<Q>;

    /// @brief The number of bits needed to represent a coefficient in Z_q
    /// @details This is the smallest integer k such that 2^k >= Q.
    static constexpr int BITLEN = std::bit_width(Q);
    /// @brief The number of bytes needed to represent a coefficient in Z_q
    /// @details This is the smallest integer k such that 8*k >= N*BITLEN.
    static constexpr size_t PACKED_BYTES = (BITLEN * N + 7) / 8;

    /// Array of N coefficients in Z_q
    std::array<CoeffType, N> coeffs;

    /// Access coefficient by index
    CoeffType& operator[](size_t idx) { return coeffs[idx]; }
    const CoeffType& operator[](size_t idx) const { return coeffs[idx]; }

    /*!
    * Packs the coefficients into a tightly-packed byte array.
    *
    * \param[out] out A std::array<uint8_t> buffer large enough to hold packed coefficients
    */
    void pack(std::array<uint8_t, PACKED_BYTES>& out) const {
        out.fill(0);
        size_t bitpos = 0;

        for (size_t i = 0; i < N; ++i) {
            uint64_t val = coeffs[i] & ((1ULL << BITLEN) - 1);

            size_t byte_index = bitpos / 8;
            int bit_offset = bitpos % 8;

            out[byte_index] |= (val << bit_offset) & 0xFF;
            if (bit_offset + BITLEN > 8)
                out[byte_index + 1] |= (val >> (8 - bit_offset)) & 0xFF;
            if (bit_offset + BITLEN > 16)
                out[byte_index + 2] |= (val >> (16 - bit_offset)) & 0xFF;

            bitpos += BITLEN;
        }
    }

    /*!
    * Unpacks a tightly-packed byte array into coefficients.
    *
    * \param[in] in A std::array<uint8_t> holding packed coefficients
    */
    void unpack(const std::array<uint8_t, PACKED_BYTES>& in) {
        size_t bitpos = 0;

        for (size_t i = 0; i < N; ++i) {
            size_t byte_index = bitpos / 8;
            int bit_offset = bitpos % 8;

            uint32_t val = (in[byte_index] >> bit_offset);
            if (bit_offset + BITLEN > 8)
                val |= (in[byte_index + 1] << (8 - bit_offset));
            if (bit_offset + BITLEN > 16)
                val |= (in[byte_index + 2] << (16 - bit_offset));

            coeffs[i] = val & ((1ULL << BITLEN) - 1);
            bitpos += BITLEN;
        }
    }
};
