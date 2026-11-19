#pragma once

#include <cstdint>
#include <cstdio>
#include <array>

namespace leyline {

template <typename T, std::size_t N>
constexpr void print_hex_dump(const std::array<T, N>& data) {
    constexpr int hex_per_line = 16;
    
    for (std::size_t i = 0; i < N; ++i) {
        printf("%02x", static_cast<unsigned char>(data[i]));
        
        if ((i + 1) % hex_per_line == 0) {
            putchar(' ');
            for (int j = 0; j < 8 - hex_per_line; ++j) {
                putchar(' ');
            }
            putchar('\n');
        } else {
            putchar(' ');
        }
    }
}

template <typename T, std::size_t N>
void print_hex_dump(const T (&data)[N]) {
    constexpr int hex_per_line = 16;
    
    for (std::size_t i = 0; i < N; ++i) {
        printf("%02x", static_cast<unsigned char>(data[i]));
        
        if ((i + 1) % hex_per_line == 0) {
            putchar('\n');
        } else {
            putchar(' ');
        }
    }
}
