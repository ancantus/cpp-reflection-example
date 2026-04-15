#pragma once

#include <meta>

/// @brief: Use in place of `enumerators_of()` to bypass bug in current gcc-trunk with the implementation in consteval contexts
template <typename T>
consteval auto enumerators_of()
{
  return std::define_static_array(std::meta::enumerators_of(^^T));
}

template <typename T, typename CTX>
consteval auto nonstatic_data_members_of(CTX ctx)
{
  return std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx));
}

