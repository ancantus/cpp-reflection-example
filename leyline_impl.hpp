#pragma once

#include <meta>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

#include "utils.hpp"

namespace leyline::detail
{
template <typename>
inline constexpr bool always_false_v = false;

template <typename T>
struct is_std_array : std::false_type
{
};

template <typename Element, std::size_t Extent>
struct is_std_array<std::array<Element, Extent>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_std_array_v = is_std_array<std::remove_cvref_t<T>>::value;

template <typename T>
struct is_span : std::false_type
{
};

template <typename Element, std::size_t Extent>
struct is_span<std::span<Element, Extent>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_span_v = is_span<std::remove_cvref_t<T>>::value;

template <typename T>
struct is_fixed_span : std::false_type
{
};

template <typename Element, std::size_t Extent>
struct is_fixed_span<std::span<Element, Extent>> : std::bool_constant<Extent != std::dynamic_extent>
{
};

template <typename T>
inline constexpr bool is_fixed_span_v = is_fixed_span<std::remove_cvref_t<T>>::value;

template <typename T>
struct is_optional : std::false_type
{
};

template <typename Value>
struct is_optional<std::optional<Value>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_optional_v = is_optional<std::remove_cvref_t<T>>::value;

template <wire::byte_order Order, typename OutIt, typename T>
constexpr void pack_impl(const T& input, OutIt& output);

template <wire::byte_order Order, typename InIt, typename T>
constexpr void unpack_impl(T& output, InIt& input);

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

template <typename OutIt>
constexpr void write_zero_bytes(std::size_t count, OutIt& output)
{
    using OutputType = std::remove_cvref_t<std::iter_value_t<OutIt>>;
    for (std::size_t i = 0U; i < count; ++i)
    {
        *output = static_cast<OutputType>(0U);
        ++output;
    }
}

template <typename InIt>
constexpr void skip_bytes(std::size_t count, InIt& input)
{
    for (std::size_t i = 0U; i < count; ++i)
    {
        ++input;
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
    else if constexpr (is_std_array_v<T>)
    {
        for (const auto& element : input)
        {
            pack_impl<Order>(element, output);
        }
    }
    else if constexpr (is_fixed_span_v<T>)
    {
        for (const auto& element : input)
        {
            pack_impl<Order>(element, output);
        }
    }
    else if constexpr (is_span_v<T>)
    {
        static_assert(always_false_v<T>, "Only fixed-extent std::span<T, N> is supported.");
    }
    else if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>)
    {
        using PointerType = std::remove_cvref_t<T>;
        using PointeeType = std::remove_cv_t<std::remove_pointer_t<PointerType>>;
        static_assert(!std::is_void_v<PointeeType>, "void pointers are not supported.");
        static_assert(!std::is_function_v<PointeeType>, "function pointers are not supported.");

        const std::uint8_t has_value = (input != nullptr) ? 1U : 0U;
        pack_impl<Order>(has_value, output);
        if (has_value != 0U)
        {
            pack_impl<Order>(*input, output);
        }
        else
        {
            write_zero_bytes(::leyline::packed_size<PointeeType>(), output);
        }
    }
    else if constexpr (is_optional_v<T>)
    {
        using ValueType = typename std::remove_cvref_t<T>::value_type;
        const std::uint8_t has_value = input.has_value() ? 1U : 0U;
        pack_impl<Order>(has_value, output);
        if (has_value != 0U)
        {
            pack_impl<Order>(*input, output);
        }
        else
        {
            write_zero_bytes(::leyline::packed_size<ValueType>(), output);
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
        static_assert(always_false_v<T>, "pack() only supports fixed-size arithmetic, arrays/spans, pointers, optionals, and recursively-reflected class members.");
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
    else if constexpr (is_std_array_v<T>)
    {
        for (auto& element : output)
        {
            unpack_impl<Order>(element, input);
        }
    }
    else if constexpr (is_fixed_span_v<T>)
    {
        using SpanType = std::remove_cvref_t<T>;
        static_assert(!std::is_const_v<typename SpanType::element_type>, "Cannot unpack into span<const T, N>.");

        for (auto& element : output)
        {
            unpack_impl<Order>(element, input);
        }
    }
    else if constexpr (is_span_v<T>)
    {
        static_assert(always_false_v<T>, "Only fixed-extent std::span<T, N> is supported.");
    }
    else if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>)
    {
        using PointerType = std::remove_cvref_t<T>;
        using PointeeType = std::remove_cv_t<std::remove_pointer_t<PointerType>>;
        static_assert(!std::is_void_v<PointeeType>, "void pointers are not supported.");
        static_assert(!std::is_function_v<PointeeType>, "function pointers are not supported.");
        static_assert(!std::is_const_v<std::remove_pointer_t<PointerType>>, "Cannot unpack into pointer-to-const.");

        std::uint8_t has_value = 0U;
        unpack_impl<Order>(has_value, input);
        if (has_value == 0U)
        {
            skip_bytes(::leyline::packed_size<PointeeType>(), input);
            output = nullptr;
        }
        else
        {
            if (output == nullptr)
            {
                std::abort();
            }
            unpack_impl<Order>(*output, input);
        }
    }
    else if constexpr (is_optional_v<T>)
    {
        using ValueType = typename std::remove_cvref_t<T>::value_type;
        std::uint8_t has_value = 0U;
        unpack_impl<Order>(has_value, input);
        if (has_value == 0U)
        {
            skip_bytes(::leyline::packed_size<ValueType>(), input);
            output.reset();
        }
        else
        {
            ValueType value{};
            unpack_impl<Order>(value, input);
            output = std::move(value);
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
        static_assert(always_false_v<T>, "unpack() only supports fixed-size arithmetic, arrays/spans, pointers, optionals, and recursively-reflected class members.");
    }
}
} // namespace leyline::detail
