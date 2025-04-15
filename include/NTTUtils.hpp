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
constexpr size_t bit_reverse(size_t x, int log_n) {
    size_t res = 0;
    for (int i = 0; i < log_n; ++i) {
        res = (res << 1) | (x & 1);
        x >>= 1;
    }
    return res;
}



template <uint64_t Q, size_t N>
struct NTTContext {
    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    using T = SmallestUInt_t<Q>;
    using MA = ModArith<Q>;

    static constexpr int log_n = std::countr_zero(N);

    std::array<size_t, N> bitrev;
    std::array<T, N / 2> forward_twiddles;
    std::array<T, N / 2> inverse_twiddles;
    T inv_n;
    T root;
    T inv_root;


    /*!
     * \brief Constructor that precomputes bit-reversal and twiddle factors.
     * \param root A primitive 2N-th root of unity modulo Q
     */
    NTTContext(uint64_t r) {

        assert(MA::pow(r, N) == 1 && "Root must be a primitive N-th root of unity for Montgomery-friendly Cooley–Tukey NTT");

        root = static_cast<T>(r);
        inv_root = static_cast<T>(MA::inv(r));

        // === Bit-reversal table ===
        for (size_t i = 0; i < N; ++i)
            bitrev[i] = bit_reverse(i, log_n);


        // === Forward twiddles (roots of unity) ===
        for (int i = 0; i < log_n; ++i) {
            forward_twiddles[i] = MA::pow(root, 1ULL << i);  // 2^i
        }
    

        // === Inverse twiddles (roots of unity for INTT) ===
        for (int i = 0; i < log_n; ++i) {
            inverse_twiddles[i] = MA::pow(inv_root, 1ULL << i);  // inv_root^{2^i}
        }

        // === Modular inverse of N (for inverse NTT normalization) ===
        inv_n = MA::inv(N);
    }
};

