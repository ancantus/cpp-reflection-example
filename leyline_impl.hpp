#pragma once

#include <meta>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
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

template <typename T>
struct checksum_region_annotation_traits
{
    static constexpr bool is_annotation = false;
    using region_tag = void;
};

template <typename RegionTag>
struct checksum_region_annotation_traits<checksum_region<RegionTag>>
{
    static constexpr bool is_annotation = true;
    using region_tag = RegionTag;
};

template <typename T>
inline constexpr bool is_checksum_region_annotation_v =
    checksum_region_annotation_traits<std::remove_cvref_t<T>>::is_annotation;

template <typename T>
struct checksum_field_annotation_traits
{
    static constexpr bool is_annotation = false;
    using algo_tag = void;
    using region_tag = void;
};

template <typename AlgoTag, typename RegionTag>
struct checksum_field_annotation_traits<checksum_field<AlgoTag, RegionTag>>
{
    static constexpr bool is_annotation = true;
    using algo_tag = AlgoTag;
    using region_tag = RegionTag;
};

template <typename T>
inline constexpr bool is_checksum_field_annotation_v =
    checksum_field_annotation_traits<std::remove_cvref_t<T>>::is_annotation;

using checksum_writer_fn = void (*)(std::span<const std::byte>, std::byte*);

struct checksum_region_entry
{
    bool present = false;
    std::string_view region_tag = {};
    std::uint16_t region_id = ::leyline::checksum_region_id_unspecified;
    std::size_t byte_offset = 0U;
    std::size_t byte_size = 0U;
};

struct checksum_field_entry
{
    bool present = false;
    std::string_view region_tag = {};
    std::uint16_t region_id = ::leyline::checksum_region_id_unspecified;
    std::size_t byte_offset = 0U;
    std::size_t byte_size = 0U;
    checksum_writer_fn writer = nullptr;
};

template <typename Tag>
consteval std::string_view checksum_tag_name()
{
    return std::meta::display_string_of(^^Tag);
}

template <wire::byte_order Order, typename AlgoTag>
constexpr void write_checksum_value(std::span<const std::byte> region_bytes, std::byte* destination)
{
    using Traits = ::leyline::checksum_traits<AlgoTag>;
    using ValueType = typename Traits::value_type;

    const ValueType checksum_value = Traits::compute(region_bytes);
    auto out_it = destination;
    pack_impl<Order>(checksum_value, out_it);
}

template <std::size_t MemberCount>
constexpr std::size_t find_next_matching_region(
    const std::array<checksum_region_entry, MemberCount>& regions,
    std::size_t field_index,
    const checksum_field_entry& field
)
{
    for (std::size_t region_index = field_index + 1U; region_index < MemberCount; ++region_index)
    {
        const checksum_region_entry& region = regions[region_index];
        if (!region.present)
        {
            continue;
        }

        if (region.region_tag != field.region_tag)
        {
            continue;
        }

        if (field.region_id == ::leyline::checksum_region_id_unspecified)
        {
            if (region.region_id != ::leyline::checksum_region_id_unspecified)
            {
                continue;
            }
        }
        else if (region.region_id != field.region_id)
        {
            continue;
        }

        return region_index;
    }

    return MemberCount;
}

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
        constexpr std::size_t member_count = nonstatic_data_members_of<T>(ctx).size();
        constexpr auto type_order = resolved_byte_order(^^T, Order);

        std::array<std::byte, ::leyline::packed_size<T>()> packed_members{};
        std::array<checksum_region_entry, member_count> regions{};
        std::array<checksum_field_entry, member_count> fields{};

        std::size_t byte_offset = 0U;
        std::size_t member_index = 0U;
        auto member_output = packed_members.begin();

        template for (constexpr auto m : nonstatic_data_members_of<T>(ctx))
        {
            using MemberType = typename[: std::meta::type_of(m) :];
            constexpr std::size_t member_size = ::leyline::packed_size<MemberType>();
            constexpr auto member_order = resolved_byte_order(m, type_order);

            template for (constexpr auto annotation : std::define_static_array(std::meta::annotations_of(m)))
            {
                using AnnotationType = typename[: std::meta::type_of(annotation) :];

                if constexpr (is_checksum_region_annotation_v<AnnotationType>)
                {
                    using NormalizedAnnotationType = std::remove_cvref_t<AnnotationType>;
                    using RegionTag = typename checksum_region_annotation_traits<NormalizedAnnotationType>::region_tag;
                    constexpr auto config = std::meta::extract<NormalizedAnnotationType>(annotation);

                    checksum_region_entry& region = regions[member_index];
                    if (region.present)
                    {
                        std::abort();
                    }

                    region.present = true;
                    region.region_tag = checksum_tag_name<RegionTag>();
                    region.region_id = config.region_id;
                    region.byte_offset = byte_offset;
                    region.byte_size = member_size;
                }
                else if constexpr (is_checksum_field_annotation_v<AnnotationType>)
                {
                    using NormalizedAnnotationType = std::remove_cvref_t<AnnotationType>;
                    using AlgoTag = typename checksum_field_annotation_traits<NormalizedAnnotationType>::algo_tag;
                    using RegionTag = typename checksum_field_annotation_traits<NormalizedAnnotationType>::region_tag;
                    using ChecksumType = typename ::leyline::checksum_traits<AlgoTag>::value_type;

                    static_assert(
                        ::leyline::packed_size<MemberType>() == ::leyline::packed_size<ChecksumType>(),
                        "checksum_field member size must match checksum_traits<AlgoTag>::value_type size."
                    );

                    checksum_field_entry& field = fields[member_index];
                    if (field.present)
                    {
                        std::abort();
                    }

                    constexpr auto config = std::meta::extract<NormalizedAnnotationType>(annotation);
                    field.present = true;
                    field.region_tag = checksum_tag_name<RegionTag>();
                    field.region_id = config.region_id;
                    field.byte_offset = byte_offset;
                    field.byte_size = member_size;
                    field.writer = &write_checksum_value<member_order, AlgoTag>;
                }
            }

            pack_impl<member_order>(input.[:m:], member_output);

            byte_offset += member_size;
            ++member_index;
        }

        for (std::size_t field_index = 0U; field_index < member_count; ++field_index)
        {
            const checksum_field_entry& field = fields[field_index];
            if (!field.present)
            {
                continue;
            }

            if (field.writer == nullptr)
            {
                std::abort();
            }

            const std::size_t region_index = find_next_matching_region(regions, field_index, field);
            if (region_index == member_count)
            {
                std::abort();
            }

            const checksum_region_entry& region = regions[region_index];
            const auto all_bytes = std::span<const std::byte>{packed_members};
            const auto region_bytes = all_bytes.subspan(region.byte_offset, region.byte_size);
            field.writer(region_bytes, packed_members.data() + field.byte_offset);
        }

        using OutputType = std::remove_cvref_t<std::iter_value_t<OutIt>>;
        for (std::byte byte : packed_members)
        {
            *output = static_cast<OutputType>(byte);
            ++output;
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
