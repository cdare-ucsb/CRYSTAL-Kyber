
#pragma once

#include <vector>
#include <cstdint>
#include <cassert>
#include <array>
#include <iostream>
#include <openssl/evp.h>  // Or use libsodium or your SHAKE128 library

#include "ModularInt.hpp"
#include "ModularPoly.hpp"
#include "ModularMatrix.hpp"

void shake128_stream(uint8_t* out, size_t outlen, const uint8_t* seed, size_t seedlen) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_shake128(), nullptr);
    EVP_DigestUpdate(mdctx, seed, seedlen);
    EVP_DigestFinalXOF(mdctx, out, outlen);
    EVP_MD_CTX_free(mdctx);
}


/**
 * Fills `output` with `required` uniformly sampled uint32_t values mod Q
 * using 32-bit rejection sampling from `buf`
 */
size_t rejection_sample(uint32_t* output, size_t required, const uint8_t* buf, size_t buflen, uint32_t Q) {
    size_t ctr = 0;
    size_t pos = 0;

    while (ctr < required && pos + 4 <= buflen) {
        uint32_t val = buf[pos]
                     | (static_cast<uint32_t>(buf[pos + 1]) << 8)
                     | (static_cast<uint32_t>(buf[pos + 2]) << 16)
                     | (static_cast<uint32_t>(buf[pos + 3]) << 24);
        pos += 4;

        if (val < Q) {
            output[ctr++] = val;
        }
    }

    return ctr;
}




/**
 * Sample n_coeffs from CBD_η using a bitstream.
 * Requires 2η bits per coefficient: total buffer size should be at least ceil(n_coeffs * 2η / 8)
 */
std::vector<uint32_t> cbd(const uint8_t* buf, size_t n_coeffs, uint8_t eta) {
    assert(eta >= 1 && eta <= 4); // practical limit for Kyber
    std::vector<uint32_t> result(n_coeffs);

    size_t bit_pos = 0;

    for (size_t i = 0; i < n_coeffs; ++i) {
        int a = 0, b = 0;
        for (uint8_t j = 0; j < eta; ++j) {
            size_t byte_index = bit_pos / 8;
            uint8_t bit_index = bit_pos % 8;
            uint8_t bit = (buf[byte_index] >> bit_index) & 1;
            a += bit;
            bit_pos++;
        }

        for (uint8_t j = 0; j < eta; ++j) {
            size_t byte_index = bit_pos / 8;
            uint8_t bit_index = bit_pos % 8;
            uint8_t bit = (buf[byte_index] >> bit_index) & 1;
            b += bit;
            bit_pos++;
        }

        result[i] = a - b;
    }

    return result;
}



std::pair<std::vector<ModularPoly>, std::vector<ModularPoly>>
generate_secret_and_error_vectors(
    const std::array<uint8_t, 32>& sigma,
    uint8_t eta,
    size_t k,
    size_t N,
    uint32_t Q
) {
    const size_t bits_per_coeff = 2 * eta;
    const size_t bytes_per_poly = (bits_per_coeff * N + 7) / 8;

    std::vector<ModularPoly> s_vector;
    std::vector<ModularPoly> e_vector;

    for (size_t i = 0; i < k; ++i) {
        std::array<uint8_t, 33> input_s;
        std::copy(sigma.begin(), sigma.end(), input_s.begin());
        input_s[32] = static_cast<uint8_t>(i);  // s: nonces 0..k-1

        std::vector<uint8_t> buf_s(bytes_per_poly);
        shake128_stream(buf_s.data(), bytes_per_poly, input_s.data(), 33);

        std::vector<uint32_t> s_i = cbd(buf_s.data(), N, eta);
        s_vector.emplace_back(ModularPoly(s_i, Q));

        // Same for e
        std::array<uint8_t, 33> input_e = input_s;
        input_e[32] = static_cast<uint8_t>(i + k);  // e: nonces k..2k-1

        std::vector<uint8_t> buf_e(bytes_per_poly);
        shake128_stream(buf_e.data(), bytes_per_poly, input_e.data(), 33);

        std::vector<uint32_t> e_i = cbd(buf_e.data(), N, eta);
        e_vector.emplace_back(ModularPoly(e_i, Q));
    }

    return {s_vector, e_vector};
}




void generate_modular_poly_entry(
    ModularPoly& poly,
    const std::array<uint8_t, 32>& rho,
    uint8_t nonce1 = 0,
    uint8_t nonce2 = 0
) {
    constexpr size_t bytes_per_val = 4;

    // Overestimate: you may reject ~20–30% of values, so get 1.5–2× N
    constexpr size_t oversample_factor = 2;
    const size_t buffer_size = oversample_factor * poly.N * bytes_per_val;

    std::array<uint8_t, 34> input;
    std::copy(rho.begin(), rho.end(), input.begin());
    input[32] = nonce1;
    input[33] = nonce2;

    std::vector<uint8_t> buf(buffer_size);
    shake128_stream(buf.data(), buffer_size, input.data(), input.size());

    std::vector<uint32_t> coeffs;
    coeffs.reserve(poly.N);

    size_t pos = 0;
    while (coeffs.size() < poly.N && pos + 4 <= buffer_size) {
        uint32_t val = buf[pos]
                     | (static_cast<uint32_t>(buf[pos + 1]) << 8)
                     | (static_cast<uint32_t>(buf[pos + 2]) << 16)
                     | (static_cast<uint32_t>(buf[pos + 3]) << 24);
        pos += 4;
 
        coeffs.push_back(val % poly.Q);
    }

    if (coeffs.size() < poly.N) {
        throw std::runtime_error("Rejection sampling failed: not enough values below Q");
    }

    poly.set_coeffs(coeffs);
}


void generate_modular_matrix(ModularMatrix& matr, const std::array<uint8_t, 32>& rho) {

    for (size_t i = 0; i < matr.rows; ++i) {
        for (size_t j = 0; j < matr.cols; ++j) {
            generate_modular_poly_entry(matr(i, j), rho, i, j);
        }
    }
}