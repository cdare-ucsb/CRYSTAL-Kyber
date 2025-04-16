#pragma once

#include "ModularInt.hpp"
#include "ModularArith.hpp"
#include "NTTUtils.hpp"



void forward_ntt(ModularInt& poly, const NTTContext& ctx) {
    auto MA = ModularArith(poly.Q);
    auto N = ctx.N;


    // === Step 1: Bit-reversal permutation ===
    for (size_t i = 0; i < N; ++i) {
        size_t j = ctx.bitrev[i];
        if (i < j) std::swap(poly[i], poly[j]);
    }


    // === Step 2: Cooley–Tukey butterfly ===
    size_t stride = 1;
    size_t twiddle_idx = ctx.log_n - 1;


    while (stride < N) {

        // Perform butterfly operations across subarrays of size `len`
        for (size_t start = 0; start < N; start += stride * 2) {

            for (size_t i = start; i < start + stride; i++) {
                uint32_t w = MA.pow( ctx.forward_twiddles[twiddle_idx], i - start);

                // Butterfly operation on positions (i+j) and (i+j+len/2)
                uint32_t u = poly[i];                       // Even-index term
                uint32_t v = MA.mul(poly[i+stride], w);                // Twisted odd-index term

                // Compute new uint32_ts in-place
                poly[i]                 = MA.add(u,  v);
                poly[i + stride]        = MA.sub(u, v);
            }
        }
        // Move to the next root of unity.
        twiddle_idx -= 1;

        stride <<= 1; // Double the stride
    }
}


void inverse_ntt(ModularInt& poly, const NTTContext& ctx) {
    auto MA = ModularArith(poly.Q);
    auto N = ctx.N;


    // === Step 1: Inverse Cooley–Tukey butterfly ===
    size_t stride = N >> 1;
    size_t twiddle_idx = 0;

    while (stride > 0) {
        for (size_t start = 0; start < N; start += stride * 2) {
            for (size_t i = start; i < start + stride; ++i) {
                uint32_t w = MA.pow(ctx.inverse_twiddles[twiddle_idx], i - start);

                uint32_t u = poly[i];
                uint32_t v = poly[i + stride];

                poly[i]             = MA.add(u, v);
                poly[i + stride]    = MA.mul(MA.sub(u,v), w);
            }
        }
        ++twiddle_idx;
        stride >>= 1;
    }

    // === Step 2: Normalize (multiply all by N^{-1}) ===
    for (size_t i = 0; i < N; ++i) {
        poly[i] = MA.mul(poly[i], ctx.inv_N);
    }

    // === Step 3: Bit-reversal permutation ===
    for (size_t i = 0; i < N; ++i) {
        size_t j = ctx.bitrev[i];
        if (i < j)
            std::swap(poly[i], poly[j]);
    }
}
