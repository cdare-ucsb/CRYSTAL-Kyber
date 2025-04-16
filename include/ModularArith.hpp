#pragma once

#include <cstdint>
#include <vector>
#include <variant>
#include <type_traits>
#include <limits>
#include <stdexcept>
#include <cmath>


class ModularArith {
public:
    uint32_t Q;
    
    ModularArith(uint32_t q) : Q(q) {
        if (q < 2) throw std::invalid_argument("Modulus Q must be >= 2");
    }

    uint32_t add(uint32_t a, uint32_t b) const {
        auto sum = a + b;
         return (sum >= Q) ? sum - Q : sum; 
    };

    uint32_t sub(uint32_t a, uint32_t b) const {
        return (a >= b) ? a - b : a + Q - b;
    }

    uint32_t mul(uint32_t a, uint32_t b) const {
        return static_cast<uint32_t>(  ( static_cast<uint64_t>(a) * static_cast<uint64_t>(b)) % Q);
    }
    uint32_t neg(uint32_t x) const {
        return  (x == 0) ? 0 : Q - x;
    }

    uint32_t pow(uint32_t base, uint32_t exp) const {
            uint64_t result = 1;
            uint64_t x = base % Q;
            while (exp > 0) {
                if (exp & 1) result = (result * x) % Q;
                x =  (x * x) % Q;
                exp >>= 1;
            }
            return static_cast<uint32_t>(result);

    }

    uint32_t inv(uint32_t a) const {
        return pow(a, Q - 2); // Assumes Q is prime
    }

    uint32_t size(uint32_t a) const {
        return (a <= (Q - 1) / 2) ? a : Q - a;
    }

    uint32_t round(uint32_t x) const {
            uint32_t q4 = Q / 4;
            return (x >= q4 && x <= Q - q4) ? 1 : 0;
    }

    bool is_primitive_nth_root(uint32_t u, uint32_t N) const {
        uint32_t res = pow(u, N);
        if (res != 1) return false;
    
        for (uint32_t d = 1; d < N; ++d) {
            if (N % d == 0) {
                uint32_t subres = pow(u, d);
                if (subres == 1) return false;
            }
        }
    
        return true;
    }

};
