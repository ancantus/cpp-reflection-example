#include <meta>
#include <array>
#include <bit>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <type_traits>

#include "utils.hpp"


// A concept to ensure the destination can receive bytes
template<typename R>
concept ByteOutputRange = std::ranges::output_range<R, uint8_t>;

template <typename>
inline constexpr bool always_false_v = false;

template <typename T, typename O>
constexpr void pack_impl(const T& input, O& output)
{
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
    {
        const auto bytes = std::bit_cast<std::array<uint8_t, sizeof(T)>>(input);
        std::ranges::copy(bytes, output);
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
        static_assert(always_false_v<T>, "pack() does not support this type");
    }
}

// will work for single bytes: and simply require the compiler to vectorize / mcopy
template <ByteOutputRange O, typename T>
constexpr void pack(const T& input, O&& output)
{
    auto outItr = std::ranges::begin(output);
    pack_impl(input, outItr);
}

template <typename T>
consteval std::size_t packed_size()
{
	// type is 'simple' and can have it's size completely computed
	if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
	{
		return std::meta::size_of(^^T);
	}
	else
	{

		constexpr auto ctx = std::meta::access_context::current();
		std::size_t accumulatedSizes{0U};
		template for (constexpr auto m : nonstatic_data_members_of<T>(ctx))
		{
			using MemberType = typename[: std::meta::type_of(m) :];
			static_assert(requires { sizeof(MemberType); }, "Member type must be complete!");
			accumulatedSizes += packed_size<MemberType>();
		}
		return accumulatedSizes;
	}
}
