/*!
 * \file ModularInt.hpp
 * \brief Header file for ModularInt class
 * \author Chros Dare
 * \date 2025-04-17
 * 
 * This file contains the definition of the ModularInt class, which represents
 * elements of the finite field extension GF(Q^N) and provides various arithmetic
 * operations, including addition, subtraction, multiplication, and division. While most
 * methods can be computed natively, the class also provides methods for NTT (Number Theoretic Transform)
 * and INTT (Inverse Number Theoretic Transform) to speed up polynomial multiplication.
 * Several other utility functions are also provided for Kyber-related operations, such as
 * the infinity/sup-norm and rounding operations.
*/


#pragma once

#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <cmath>
#include <bit> 
#include <cmath>
#include <variant>
#include <cassert>


#include "ModularInt.hpp"
#include "NTTUtils.hpp"



/*!
* \brief Class representing an element of the finite field extension GF(Q^N).
* 
* This class provides methods for polynomial arithmetic, including addition,
* subtraction, and multiplication. It also supports NTT (Number Theoretic Transform)
* and INTT (Inverse Number Theoretic Transform) for efficient polynomial multiplication. 
* The class is designed to work with a modulus Q and a polynomial degree N.
*/
class ModularPoly {
public:

    // using CoeffType = SmallestUInt_t<Q>;
    // using MA = ModArith<Q>;

    uint32_t Q;
    size_t N;
    
    std::vector<ModularInt> coeffs;
    bool NTTed = false;

    const NTTContext* ctx = nullptr;

    

    ModularPoly(size_t n, uint32_t q , const NTTContext* ctx = nullptr)
        : Q(q), N(n), coeffs(n, ModularInt(0, q)), ctx(ctx), NTTed(false)
    {
        if (Q == 0) throw std::invalid_argument("ModularPoly constructed with Q = 0");
    }

    ModularPoly(const std::vector<uint32_t>& coeffs_in, uint32_t q, const NTTContext* ctx = nullptr)
        : Q(q), N(coeffs_in.size()), NTTed(false), ctx(ctx)
    {
        if (Q == 0) throw std::invalid_argument("ModularPoly constructed with Q = 0");
        
        this->coeffs.resize(N);
        for (size_t i = 0; i < N; ++i) {
            this->coeffs[i] = ModularInt(coeffs_in[i], q);
        }
    }

    ModularPoly(const std::vector<ModularInt>& coeffs, uint32_t q, const NTTContext* ctx = nullptr)
        : Q(q), coeffs(coeffs), ctx(ctx)
    {
        N = coeffs.size();
        NTTed = false;
    }


    void set_coeffs(const std::vector<uint32_t>& coeffs) {
        if (coeffs.size() != N) {
            throw std::invalid_argument("Coefficient size does not match polynomial size.");
        }
        for (size_t i = 0; i < N; ++i) {
            this->coeffs[i] = ModularInt(coeffs[i], Q);
        }
    }

    /*!
     * \brief Accessor for coefficients
     * \param idx Index of the coefficient to access
     * \return Reference to the coefficient at index idx
     * 
     * Standard accessor for coefficients, allowing the [] operator to be used on ModularPoly objects.
     */
    ModularInt& operator[](size_t idx) {
        return coeffs[idx];
    }

    /*!
     * \brief Constant accessor for coefficients
     * \param idx Index of the coefficient to access
     * \return Reference to the coefficient at index idx
     * 
     * Overloaded constant accessor for coefficients, allowing the [] operator to be used on ModularPoly objects.
     */
    const ModularInt& operator[](size_t idx) const {
        return coeffs[idx];
    }

    
    std::string to_string() const {
        std::string result;
        if (N < 4) {
            result = coeffs[0].to_string() + " + " + coeffs[1].to_string() + "x + " + coeffs[2].to_string() + "x^2 + " + coeffs[3].to_string() + "x^3";
        }
        else {
            result = coeffs[0].to_string() + " + " + coeffs[1].to_string() + "x +...+ " + coeffs[N - 1].to_string() + "x^" + std::to_string(N - 1);
        }

        return result;
    }

    /*!
     * \brief Get the size of the polynomial
     * \return Size of the polynomial
     * 
     * Returns the number of coefficients in the polynomial, which is simply one of the member variables N.
     */
    size_t size() const {
        return N;
    }


    /*!
    * \brief Define the sum of polynomials as the sum of their coefficients modulo Q
    * \return A ModularPoly object representing the sum of the two polynomials
    * 
    * In the finite field extension, the sum is taken component-wise. Thus linear terms are 
    * added together in ZZ_Q, quadratic terms are added together in ZZ_Q, etc. The result is a new polynomial
    * with the same size and modulus as the original polynomials.
    */
    ModularPoly operator+(const ModularPoly& other) const {
        if (Q != other.Q || N != other.N) {
            throw std::invalid_argument("Polynomials must have the same modulus and size.");
        }

        if (NTTed && !other.NTTed || !NTTed && other.NTTed) {
            throw std::invalid_argument("The two polynomials are in opposite NTT domains");
        }

        ModularPoly result(N, Q, ctx);
        for (size_t i = 0; i < N; ++i) {
            result[i] = coeffs[i] + other[i];
        }
        return result;
    }
    ModularPoly operator-(const ModularPoly& other) const {
        if (Q != other.Q || N != other.N) {
            throw std::invalid_argument("Polynomials must have the same modulus and size.");
        }

        if (NTTed && !other.NTTed || !NTTed && other.NTTed) {
            throw std::invalid_argument("The two polynomials are in opposite NTT domains");
        }

        ModularPoly result(N, Q, ctx);
        for (size_t i = 0; i < N; ++i) {
            result[i] = coeffs[i] - other[i];
        }
        return result;
    }


    ModularPoly operator*(ModularPoly& other) {
        if (Q != other.Q || N != other.N) {
            throw std::invalid_argument("Polynomials must have the same modulus and size.");
        }

        // === Step 1: Move to NTT Domain to convert convolution to pointwise mult ===
        if (!NTTed) {
            NTT();
        }
        if (!other.NTTed) {
            other.NTT();
        }

        // === Step 2: Pointwise multiplication ===
        ModularPoly result(N, Q, ctx);
        for (size_t i = 0; i < N; ++i) {
            result[i] = coeffs[i] * other[i];
        }
        return result;

        // === Step 3: Move back to time domain ===

        result.INTT();

        return result;

    }

    
    uint32_t sup_norm() const {
        uint32_t max_val = 0;

        for (auto c : coeffs) {
            uint32_t size = (c.val > Q / 2) ? (Q - c.val) : c.val;
            if (size > max_val) max_val = size;
        }

        return max_val;
    }


    ModularPoly round() {
        std::vector<uint32_t> rounded_coeffs(N);
        for (size_t i = 0; i < N; ++i) {
            rounded_coeffs[i] = coeffs[i].round();
        }
        return ModularPoly(rounded_coeffs, Q, ctx);
    }


    void NTT() {
        if (NTTed) return;

        if (ctx == nullptr) {
            throw std::runtime_error("NTTContext is not set.");
        }

        // === Step 1: Bit-reversal permutation ===
        for (size_t i = 0; i < N; ++i) {
            size_t j = ctx->bitrev[i];

            if (i < j) std::swap(coeffs[i], coeffs[j]);
        }


        // === Step 2: Cooley–Tukey butterfly ===
        size_t stride = 1;
        size_t twiddle_idx = ctx->log_n - 1;


        while (stride < N) {

            // Perform butterfly operations across subarrays of size `len`
            for (size_t start = 0; start < N; start += stride * 2) {

                for (size_t i = start; i < start + stride; i++) {

                    ModularInt w = ctx->forward_twiddles[twiddle_idx].pow(i - start);

                    // Butterfly operation on positions (i+j) and (i+j+len/2)
                    ModularInt u = coeffs[i];                       // Even-index term
                    ModularInt v = coeffs[i+stride]*w;                // Twisted odd-index term

                    // Compute new uint32_ts in-place
                    coeffs[i]                 = u + v;
                    coeffs[i + stride]        = u - v;
                }
            }
            // Move to the next root of unity.
            twiddle_idx -= 1;

            stride <<= 1; // Double the stride
        }

        NTTed = true;
    }

    void INTT() {

        if (!NTTed) return;

        if (ctx == nullptr) {
            throw std::runtime_error("NTTContext is not set.");
        }


         // === Step 1: Inverse Cooley–Tukey butterfly ===
        size_t stride = N >> 1;
        size_t twiddle_idx = 0;

        while (stride > 0) {
            for (size_t start = 0; start < N; start += stride * 2) {
                for (size_t i = start; i < start + stride; ++i) {
                    ModularInt w = ctx->inverse_twiddles[twiddle_idx].pow(i - start);

                    ModularInt u = coeffs[i];
                    ModularInt v = coeffs[i + stride];

                    coeffs[i]             = u + v;
                    coeffs[i + stride]    = (u - v) * w;
                }
            }
            ++twiddle_idx;
            stride >>= 1;
        }

        // === Step 2: Normalize (multiply all by N^{-1}) ===
        for (size_t i = 0; i < N; ++i) {
            coeffs[i] = coeffs[i] * ModularInt(N, Q).inv();
        }

        // === Step 3: Bit-reversal permutation ===
        for (size_t i = 0; i < N; ++i) {
            size_t j = ctx->bitrev[i];
            if (i < j)
                std::swap(coeffs[i], coeffs[j]);
        }

        NTTed = false;
    }


    /*!
     * \brief Pack the polynomial coefficients into a byte array
     * \return A vector of uint8_t representing the packed coefficients
     * 
     * This method packs the polynomial coefficients into a byte array, which can be used for serialization or transmission.
     */
    std::vector<uint8_t> pack() const {
        size_t bitlen = std::bit_width(Q);
        size_t out_size = (bitlen * N + 7) / 8;;
        std::vector<uint8_t> out(out_size, 0);
        size_t bitpos = 0;

        for (size_t i = 0; i < N; ++i) {
            uint64_t val = coeffs[i].val & ((1ULL << bitlen) - 1);

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

    /*!
     * \brief Unpack the polynomial coefficients from a byte array
     * \param in A vector of uint8_t representing the packed coefficients
     * 
     * This method unpacks the polynomial coefficients from a byte array, which can be used for deserialization or reception. 
     * This method overwrites the current coefficients of the polynomial.
     */
    void unpack(const std::vector<uint8_t>& in) {
        size_t bitlen = std::bit_width(Q);
        size_t bitpos = 0;

        for (size_t i = 0; i < N; ++i) {
            size_t byte_index = bitpos / 8;
            int bit_offset = bitpos % 8;

            uint32_t val = (in[byte_index] >> bit_offset);
            if (bit_offset + bitlen > 8)
                val |= (in[byte_index + 1] << (8 - bit_offset));
            if (bit_offset + bitlen > 16)
                val |= (in[byte_index + 2] << (16 - bit_offset));

            coeffs[i].val = val & ((1ULL << bitlen) - 1);
            bitpos += bitlen;
        }

    }

};
