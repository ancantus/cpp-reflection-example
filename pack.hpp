#include <meta>
#include <ranges>

#include "utils.hpp"


// A concept to ensure the destination can receive bytes
template<typename R>
concept ByteOutputRange = std::ranges::output_range<R, uint8_t>;

// will work for single bytes: and simply require the compiler to vectorize / mcopy
template <ByteOutputRange O, typename T>
void pack(const T& input, O&& output)
{


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
