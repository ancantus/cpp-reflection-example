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

consteval wire::byte_order resolved_byte_order(std::meta::info subject, wire::byte_order fallback)
{
    const auto annotations = std::meta::annotations_of_with_type(subject, ^^wire::endian);
    if (annotations.empty())
    {
        return fallback;
    }

    return std::meta::extract<wire::endian>(annotations[0]).value;
}

template <wire::byte_order Order>
consteval bool should_reverse_byte_order()
{
    static_assert(
        std::endian::native == std::endian::little || std::endian::native == std::endian::big,
        "Mixed-endian targets are not supported."
    );

    if constexpr (Order == wire::byte_order::little)
    {
        return std::endian::native == std::endian::big;
    }
    else
    {
        return std::endian::native == std::endian::little;
    }
}

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

template <wire::byte_order Order = wire::byte_order::little, typename OutIt, typename T>
constexpr void pack_impl(const T& input, OutIt& output)
{
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
    {
        const auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(input);
        constexpr bool reverse_byte_order = should_reverse_byte_order<Order>();
        for (std::size_t i = 0; i < bytes.size(); ++i)
        {
            const std::size_t byte_index = reverse_byte_order ? (bytes.size() - 1U - i) : i;
            // cast here to allow end user usage of any byte type they wish
            using OutputType = std::remove_cvref_t<std::iter_value_t<OutIt>>;
            *output = static_cast<OutputType>(bytes[byte_index]);
            ++output;
        }
    }
    else if constexpr (std::is_array_v<T>)
    {
        for (const auto& element : input)
        {
            pack_impl<Order>(element, output);
        }
    }
    else if constexpr (std::is_class_v<T>)
    {
        constexpr auto ctx = std::meta::access_context::current();
        constexpr auto type_order = resolved_byte_order(^^T, Order);
        template for (constexpr auto m : nonstatic_data_members_of<T>(ctx))
        {
            constexpr auto member_order = resolved_byte_order(m, type_order);
            pack_impl<member_order>(input.[:m:], output);
        }
    }
    else
    {
        static_assert(always_false_v<T>, "pack() only supports arithmetic, enums, arrays, and recursively-reflected class members.");
    }
}

template <wire::byte_order Order = wire::byte_order::little, typename InIt, typename T>
constexpr void unpack_impl(T& output, InIt& input)
{
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
    {
        std::array<std::uint8_t, sizeof(T)> bytes{};
        constexpr bool reverse_byte_order = should_reverse_byte_order<Order>();
        for (std::size_t i = 0; i < bytes.size(); ++i)
        {
            const std::size_t byte_index = reverse_byte_order ? (bytes.size() - 1U - i) : i;
            bytes[byte_index] = byte_to_u8(*input);
            ++input;
        }
        output = std::bit_cast<T>(bytes);
    }
    else if constexpr (std::is_array_v<T>)
    {
        for (auto& element : output)
        {
            unpack_impl<Order>(element, input);
        }
    }
    else if constexpr (std::is_class_v<T>)
    {
        constexpr auto ctx = std::meta::access_context::current();
        constexpr auto type_order = resolved_byte_order(^^T, Order);
        template for (constexpr auto m : nonstatic_data_members_of<T>(ctx))
        {
            constexpr auto member_order = resolved_byte_order(m, type_order);
            unpack_impl<member_order>(output.[:m:], input);
        }
    }
    else
    {
        static_assert(always_false_v<T>, "unpack() only supports arithmetic, enums, arrays, and recursively-reflected class members.");
    }
}
} // namespace leyline::detail
