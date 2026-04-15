#include <print>
#include <meta>
#include <type_traits>
#include <string>
#include <cstdint>

#include "utils.hpp"

enum class Color { Red, Green, Blue };

template <typename E>
  requires std::is_enum_v<E>
constexpr std::string enum_to_string(E value) {
  template for (constexpr auto e : enumerators_of<E>()) {
    if (value == [:e:]) {
      return std::string(std::meta::identifier_of(e));
    }
  }

  return "<unnamed>";
}

template <typename E>
  requires std::is_enum_v<E>
constexpr std::size_t enum_count(E) {
  return std::meta::enumerators_of(^^E).size();
}

int main()
{
  std::println("color 2 = {}", enum_to_string(Color::Red));
  std::println("color 2 = {}", enum_to_string(Color::Green));
  std::println("color 3 = {}", enum_to_string(Color::Blue));
  std::println("color 4 = {}", enum_to_string(static_cast<Color>(255)));
  return 0;
}

