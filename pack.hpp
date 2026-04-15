#pragma once

#include <meta>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <type_traits>
#include <utility>

#include "utils.hpp"

template <typename R>
concept ByteOutputRange = std::ranges::output_range<R, uint8_t>;

template <typename R>
concept ByteInputRange =
    std::ranges::input_range<R> &&
    (
        std::same_as<std::remove_cvref_t<std::ranges::range_reference_t<R>>, std::byte> ||
        (
            std::integral<std::remove_cvref_t<std::ranges::range_reference_t<R>>> &&
            sizeof(std::remove_cvref_t<std::ranges::range_reference_t<R>>) == 1U
        )
    );

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
        const auto bytes = std::bit_cast<std::array<uint8_t, sizeof(T)>>(input);
        for (uint8_t byte : bytes)
        {
            *output = byte;
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
        std::array<uint8_t, sizeof(T)> bytes{};
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

template <ByteOutputRange O, typename T>
constexpr void pack(const T& input, O&& output)
{
    auto out_it = std::ranges::begin(output);
    pack_impl(input, out_it);
}

template <ByteInputRange I, typename T>
constexpr void unpack(T& output, I&& input)
{
    auto in_it = std::ranges::begin(input);
    unpack_impl(output, in_it);
}

template <typename T, ByteInputRange I>
constexpr T unpack(I&& input)
{
    T output{};
    unpack(output, std::forward<I>(input));
    return output;
}

template <typename T>
consteval std::size_t packed_size()
{
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
    {
        return std::meta::size_of(^^T);
    }
    else if constexpr (std::is_array_v<T>)
    {
        return std::extent_v<T> * packed_size<std::remove_extent_t<T>>();
    }
    else
    {
        constexpr auto ctx = std::meta::access_context::current();
        std::size_t accumulated_sizes{0U};
        template for (constexpr auto m : nonstatic_data_members_of<T>(ctx))
        {
            using MemberType = typename[: std::meta::type_of(m) :];
            static_assert(requires { sizeof(MemberType); }, "Member type must be complete!");
            accumulated_sizes += packed_size<MemberType>();
        }
        return accumulated_sizes;
    }
}
