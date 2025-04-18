/*!
 * \file ModularInt.hpp
 * \brief Header file for ModularInt class
 * \author Chris Dare
 * \date 2025-04-17
 *
 * This file contains the definition of the ModularInt class, which represents
 * integers in ZZ_Q (i.e. the cyclic group of order Q). The class provides various arithmetic
 * operations, including addition, subtraction, multiplication, and division.
 * It also includes methods for exponentiation, modular inverse, and checking if the
 * integer is a primitive N-th root of unity.
 *
*/
#pragma once
#include <cassert>

/*!
 * \brief Class representing integers in ZZ_Q
 *
 * This class provides modular arithmetic operations for integers in the range [0, Q).
 * It supports addition, subtraction, multiplication, division, and exponentiation.
 * It also provides methods for checking if the integer is a primitive N-th root of unity.
 */
class ModularInt {
public:

    uint32_t Q;
    uint32_t val;

    ModularInt() : Q(std::numeric_limits<uint32_t>::max()), val(0) {}

    ModularInt(uint32_t v, uint32_t q) : Q(q), val(v % q) {
        if (q == 0) throw std::invalid_argument("Q cannot be zero");
    }


    ModularInt operator+(const ModularInt& other) const {
        assert(Q == other.Q);
        return ModularInt((val + other.val) % Q, Q);
    }
    
    ModularInt operator-(const ModularInt& other) const {
        assert(Q == other.Q);
        return ModularInt((val + Q - other.val) % Q, Q);
    }

    ModularInt operator*(const ModularInt& other) const {
        assert(Q == other.Q);
        return ModularInt((static_cast<uint64_t>(val) * other.val) % Q, Q);
    }

    ModularInt operator/(const ModularInt& other) const {
        assert(Q == other.Q);
        return ModularInt((static_cast<uint64_t>(val) * other.inv().val) % Q, Q);
    }
    ModularInt inv() const {
        assert(Q > 1);
        return pow(Q - 2);
    }

    explicit operator uint32_t() const {
        return val;
    }


    ModularInt pow(uint32_t exp) const {
        uint64_t base = static_cast<uint64_t>(val);
        uint64_t result = 1;
        uint64_t x = base % Q;
        while (exp > 0) {
            if (exp & 1) result = (result * x) % Q;
            x = (x * x) % Q;
            exp >>= 1;
        }
        return ModularInt(static_cast<uint32_t>(result), Q);
    }

    uint32_t size() const {
        return (val <= (Q - 1) / 2) ? val : Q - val;
    }
    uint32_t round() const {
        uint32_t q4 = Q / 4;
        return (val >= q4 && val <= Q - q4) ? 1 : 0;
    }

    // Compare with another ModularInt
    bool operator==(const ModularInt& other) const {
        return Q == other.Q && val == other.val;
    }

    // Compare with uint32_t
    bool operator==(uint32_t other_val) const {
        return val == (other_val % Q);
    }

    // Compare with int (safe version)
    bool operator==(int other_val) const {
        if (other_val < 0)
            return val == (Q - (static_cast<uint32_t>(-other_val) % Q));
        return val == (static_cast<uint32_t>(other_val) % Q);
    }

    bool operator!=(const ModularInt& other) const { return !(*this == other); }
    bool operator!=(uint32_t other_val) const { return !(*this == other_val); }
    bool operator!=(int other_val) const { return !(*this == other_val); }


    std::string to_string() const {
        return std::to_string(val); // Assuming `value` is the internal representation
    }

    bool is_primitive_nth_root(uint32_t N) const {
        if (pow(N) != 1) return false;
    
        for (uint32_t d = 1; d < N; ++d) {
            if (N % d == 0) {
                if (pow(d) == 1) return false;
            }
        }
    
        return true;
    }

};