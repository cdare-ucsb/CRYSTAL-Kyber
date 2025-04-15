#pragma once

#include <cstdint>
#include <type_traits>
#include "SmallestUInt.hpp"

template <uint64_t Q>
struct ModArith {
    using T = SmallestUInt_t<Q>;
    using WideT = std::conditional_t<(sizeof(T) <= 2), uint32_t, uint64_t>;

    static constexpr T add(T a, T b) {
        T sum = a + b;
        return (sum >= Q) ? sum - Q : sum;
    }

    static constexpr T sub(T a, T b) {
        return (a >= b) ? a - b : a + Q - b;
    }

    static constexpr T mul(T a, T b) {
        return static_cast<T>(static_cast<WideT>(a) * b % Q);
    }

    static constexpr T pow(T base, uint64_t exp) {
        WideT result = 1;
        WideT b = base % Q;

        while (exp > 0) {
            if (exp & 1)
                result = (result * b) % Q;
            b = (b * b) % Q;
            exp >>= 1;
        }
        return static_cast<T>(result);
    }

    static constexpr T inv(T a) {
        return pow(a, Q - 2); // Assumes Q is prime
    }

    static constexpr T neg(T a) {
        return (a == 0) ? 0 : Q - a;
    }

    /*!
    * \brief Computes the size of an element in Z_Q, defined as the absolute value
    *        of its symmetric representative in the range [-(Q-1)/2, (Q-1)/2]
    *
    * \param x An element of Z_Q
    * \return The size (non-negative distance from 0) in Z_Q
    */
    static constexpr T size(T x) {
        return (x <= (Q - 1) / 2) ? x : Q - x;
    }


    /*!
     * \brief Rounds an element x ∈ Z_Q to {0, 1} depending on closeness to 0 or Q/2
     *
     * \return 0 if x ∈ [0, Q/4) ∪ (3Q/4, Q)
     *         1 if x ∈ [Q/4, 3Q/4]
     */
    static constexpr T round(T x) {
        constexpr T quarter_q = Q / 4;
        return (x >= quarter_q && x <= Q - quarter_q) ? 1 : 0;
    }

    // Compute all proper divisors of N (excluding N)
    static std::vector<uint64_t> proper_divisors(uint64_t N) {
        std::vector<uint64_t> divisors;
        for (uint64_t i = 1; i * i <= N; ++i) {
            if (N % i == 0) {
                divisors.push_back(i);
                if (i != 1 && i != N / i) {
                    divisors.push_back(N / i);
                }
            }
        }
        return divisors;
    }

    static bool is_primitive_nth_root(T uroot, uint64_t N) {
        if (pow(uroot, N) != 1)
            return false;
    
        auto divisors = proper_divisors(N);
        for (uint64_t d : divisors) {
            if (d == N) continue;
            if (pow(uroot, d) == 1)
                return false;
        }
    
        return true;
    }
};