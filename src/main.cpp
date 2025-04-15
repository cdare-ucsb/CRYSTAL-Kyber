#include "Poly.hpp"
#include <iostream>

int main() {
    constexpr uint64_t Q = 3329;
    constexpr size_t N = 256;

    using MyPoly = Poly<Q, N>;

    MyPoly p;
    for (size_t i = 0; i < N; ++i)
        p[i] = rand() % Q;

    std::array<uint8_t, MyPoly::PACKED_BYTES> buffer;
    p.pack(buffer);

    MyPoly recovered;
    recovered.unpack(buffer);

    for (size_t i = 0; i < 5; ++i)
        std::cout << p[i] << " <-> " << recovered[i] << "\n";
}
