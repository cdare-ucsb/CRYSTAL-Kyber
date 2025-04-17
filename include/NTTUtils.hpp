#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <bit>
#include "ModularInt.hpp"



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
    uint32_t Q;
    ModularInt root, inv_root;

    size_t N, log_n;
    
    std::vector<size_t> bitrev;

    std::vector<ModularInt> forward_twiddles;
    std::vector<ModularInt> inverse_twiddles;

    NTTContext() = default;

    NTTContext(uint32_t Q_, size_t N_, uint32_t root_)
        : Q(Q_), N(N_)  {

        root = ModularInt(root_, Q);
        inv_root = root.inv();

        log_n = std::countr_zero(N_);

        // === Bit-reversal table ===
        bitrev.resize(N_);
        for (size_t i = 0; i < N_; ++i)
            bitrev[i] = bit_reverse(i, log_n);

        // === Twiddle factors ===
        forward_twiddles.resize(log_n);
        inverse_twiddles.resize(log_n);

        for (size_t i = 0; i < log_n; ++i) {
            forward_twiddles[i] = root.pow(1ULL << i);
            inverse_twiddles[i] = inv_root.pow(1ULL << i);
        }
    }
    
};