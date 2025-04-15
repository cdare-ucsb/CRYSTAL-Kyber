#pragma once

#include <cstdint>
#include <type_traits>
#include <limits>

// A type trait that selects the smallest unsigned integer type
// capable of holding any value in Z_q (i.e., 0 <= x < Q)
template <uint64_t Q>
struct SmallestUInt {
    static_assert(Q > 0, "Modulus Q must be positive.");

    using type = std::conditional_t<
        (Q <= std::numeric_limits<uint8_t>::max()), uint8_t,
        std::conditional_t<
            (Q <= std::numeric_limits<uint16_t>::max()), uint16_t,
            std::conditional_t<
                (Q <= std::numeric_limits<uint32_t>::max()), uint32_t,
                uint64_t
            >
        >
    >;
};

// Shorthand alias template
template <uint64_t Q>
using SmallestUInt_t = typename SmallestUInt<Q>::type;
