#pragma once

#include <meta>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "utils.hpp"

namespace leyline::detail
{
template <typename>
inline constexpr bool always_false_v = false;

template <typename T>
constexpr std::uint8_t byte_to_u8(T value)
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

template <typename OutIt, typename T>
constexpr void pack_impl(const T& input, OutIt& output)
{
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
    {
        const auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(input);
        for (const auto byte : bytes)
        {
            // cast here to allow end user usage of any byte type they wish
            using OutputType = std::remove_cvref_t<std::iter_value_t<OutIt>>;
            *output = static_cast<OutputType>(byte);
            ++output;
        }
    }
    else if constexpr (std::is_array_v<T>)
    {
        for (const auto& element : input)
        {
            pack_impl(element, output);
        }
    }
    else if constexpr (std::is_class_v<T>)
    {
        constexpr auto ctx = std::meta::access_context::current();
        template for (constexpr auto m : nonstatic_data_members_of<T>(ctx))
        {
            pack_impl(input.[:m:], output);
        }
    }
    else
    {
        static_assert(always_false_v<T>, "pack() only supports arithmetic, enums, arrays, and recursively-reflected class members.");
    }
}

template <typename InIt, typename T>
constexpr void unpack_impl(T& output, InIt& input)
{
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
    {
        std::array<std::uint8_t, sizeof(T)> bytes{};
        for (std::size_t i = 0; i < bytes.size(); ++i)
        {
            bytes[i] = byte_to_u8(*input);
            ++input;
        }
        output = std::bit_cast<T>(bytes);
    }
    else if constexpr (std::is_array_v<T>)
    {
        for (auto& element : output)
        {
            unpack_impl(element, input);
        }
    }
    else if constexpr (std::is_class_v<T>)
    {
        constexpr auto ctx = std::meta::access_context::current();
        template for (constexpr auto m : nonstatic_data_members_of<T>(ctx))
        {
            unpack_impl(output.[:m:], input);
        }
    }
    else
    {
        static_assert(always_false_v<T>, "unpack() only supports arithmetic, enums, arrays, and recursively-reflected class members.");
    }
}
} // namespace leyline::detail
