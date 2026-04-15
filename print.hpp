#pragma once

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <type_traits>
#include <vector>

template <typename T>
concept ByteLike =
    std::same_as<std::remove_cvref_t<T>, std::byte> ||
    (std::integral<std::remove_cvref_t<T>> && sizeof(std::remove_cvref_t<T>) == 1U);

template <typename T>
constexpr std::uint8_t to_u8(T value)
{
    if constexpr (std::same_as<std::remove_cvref_t<T>, std::byte>)
    {
        return std::to_integer<std::uint8_t>(value);
    }
    else
    {
        return static_cast<std::uint8_t>(value);
    }
}

template <std::ranges::input_range R>
    requires ByteLike<std::ranges::range_reference_t<R>>
void print_hex_dump(R&& range, std::ostream& out = std::cout, std::size_t bytes_per_line = 16)
{
    if (bytes_per_line == 0)
    {
        return;
    }

    std::vector<std::uint8_t> line(bytes_per_line);
    std::size_t line_size = 0;
    std::size_t total_size = 0;

    auto flush_line = [&](std::size_t line_offset) {
        auto old_flags = out.flags();
        auto old_fill = out.fill();

        out << std::hex << std::setfill('0') << std::setw(8) << line_offset << "  ";

        for (std::size_t i = 0; i < bytes_per_line; ++i)
        {
            if (i < line_size)
            {
                out << std::setw(2) << static_cast<unsigned>(line[i]) << ' ';
            }
            else
            {
                out << "   ";
            }

            if (i == 7)
            {
                out << ' ';
            }
        }

        out << " |";
        for (std::size_t i = 0; i < line_size; ++i)
        {
            const unsigned char byte = line[i];
            out << (std::isprint(byte) != 0 ? static_cast<char>(byte) : '.');
        }

        for (std::size_t i = line_size; i < bytes_per_line; ++i)
        {
            out << ' ';
        }

        out << "|\n";
        out.flags(old_flags);
        out.fill(old_fill);
    };

    for (auto&& value : range)
    {
        line[line_size] = to_u8(value);
        ++line_size;
        ++total_size;

        if (line_size == bytes_per_line)
        {
            flush_line(total_size - line_size);
            line_size = 0;
        }
    }

    if (line_size != 0)
    {
        flush_line(total_size - line_size);
    }
}
