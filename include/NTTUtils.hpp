#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <bit>
#include "ModularArith.hpp"



/*!
 * \brief Computes the bit-reversal of an integer `x` with respect to `log_n` bits.
 *
 * \param x The integer to reverse
 * \param log_n The number of bits to reverse (i.e., log2 of the NTT size)
 * \return The bit-reversed integer
 */
constexpr size_t bit_reverse(size_t x, uint32_t log_n) {
    size_t res = 0;
    for (int i = 0; i < log_n; ++i) {
        res = (res << 1) | (x & 1);
        x >>= 1;
    }
    return res;
}




struct NTTContext {
    uint32_t Q, inv_N, root, inv_root;

    size_t N, log_n;
    
    std::vector<size_t> bitrev;

    std::vector<uint32_t> forward_twiddles;
    std::vector<uint32_t> inverse_twiddles;

    NTTContext() = default;

    NTTContext(uint32_t Q_, size_t N_, uint32_t root_)
        : Q(Q_), N(N_), root(root_) {

        auto mod_arith = ModularArith(Q_);

        inv_root = mod_arith.inv(root);
        inv_N = mod_arith.inv(N);

        log_n = std::countr_zero(N);

        // === Bit-reversal table ===
        bitrev.resize(N);
        for (size_t i = 0; i < N; ++i)
            bitrev[i] = bit_reverse(i, log_n);

        // === Twiddle factors ===
        forward_twiddles.resize(log_n);
        inverse_twiddles.resize(log_n);

        for (size_t i = 0; i < log_n; ++i) {
            forward_twiddles[i] = mod_arith.pow(root, 1ULL << i);
            inverse_twiddles[i] = mod_arith.pow(inv_root, 1ULL << i);
        }
    }
    
};