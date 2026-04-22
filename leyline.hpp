#pragma once

#include <meta>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>

#include "utils.hpp"

namespace leyline::wire
{
enum class byte_order : std::uint8_t
{
    little,
    big
};

struct endian
{
    byte_order value;

    constexpr bool operator==(const endian&) const noexcept = default;
};

inline constexpr endian little{byte_order::little};
inline constexpr endian big{byte_order::big};
} // namespace leyline::wire

namespace leyline
{
template <typename T>
consteval std::size_t packed_size();
} // namespace leyline

#include "leyline_impl.hpp"

namespace leyline
{
template <typename T>
concept ByteLike =
    std::same_as<std::remove_cvref_t<T>, std::byte> ||
    (
        std::integral<std::remove_cvref_t<T>> &&
        sizeof(std::remove_cvref_t<T>) == 1U
    );

template <typename R>
concept ByteOutputRange = std::ranges::output_range<R, std::ranges::range_reference_t<R>> 
    && ByteLike<std::ranges::range_reference_t<R>>;

template <typename R>
concept ByteInputRange =
    std::ranges::input_range<R> && ByteLike<std::ranges::range_reference_t<R>>;

template <ByteOutputRange O, typename T>
constexpr void pack(const T& input, O&& output)
{
    auto out_it = std::ranges::begin(output);
    detail::pack_impl(input, out_it);
}

template <ByteInputRange I, typename T>
constexpr void unpack(T& output, I&& input)
{
    auto in_it = std::ranges::begin(input);
    detail::unpack_impl(output, in_it);
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
    else if constexpr (detail::is_std_array_v<T>)
    {
        using ArrayType = std::remove_cvref_t<T>;
        using ElementType = typename ArrayType::value_type;
        return std::tuple_size_v<ArrayType> * packed_size<ElementType>();
    }
    else if constexpr (detail::is_span_v<T>)
    {
        using SpanType = std::remove_cvref_t<T>;
        static_assert(SpanType::extent != std::dynamic_extent, "Only fixed-extent std::span<T, N> is supported.");
        using ElementType = std::remove_cv_t<typename SpanType::element_type>;
        return SpanType::extent * packed_size<ElementType>();
    }
    else if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>)
    {
        using PointeeType = std::remove_cv_t<std::remove_pointer_t<std::remove_cvref_t<T>>>;
        static_assert(!std::is_void_v<PointeeType>, "void pointers are not supported.");
        static_assert(!std::is_function_v<PointeeType>, "function pointers are not supported.");
        return 1U + packed_size<PointeeType>();
    }
    else if constexpr (detail::is_optional_v<T>)
    {
        using ValueType = typename std::remove_cvref_t<T>::value_type;
        return 1U + packed_size<ValueType>();
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
} // namespace leyline
