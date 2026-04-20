#pragma once

#include <meta>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <type_traits>
#include <utility>

#include "leyline_impl.hpp"
#include "utils.hpp"

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
