/************************************************************************
 * Copyright (c) 2025 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file static_array.hpp
 * @version 1.0
 * @date 4/4/25
 * @brief Static array utilitites
 *
 * These utilities are workarounds that are needed by now due to the proposal implementation status
 */

#pragma once

#include <experimental/meta>

namespace rflect {

template<std::size_t N>
consteval auto static_iota() {
  return define_static_array(template_arguments_of(dealias(^^std::make_index_sequence<N>)) | std::views::drop(1));
}

consteval auto operator""_ss(char const* str, [[maybe_unused]] size_t length) -> char const* {
  return std::meta::define_static_string(str);
}

struct to_static_fn {
  template <std::ranges::input_range R>
  consteval auto operator()(R&& r) const {
    return define_static_array(r);
  }

  template <std::ranges::input_range R>
  friend consteval auto operator|(R&& r, const to_static_fn& self) {
    return self(std::forward<R>(r));
  }
};

consteval to_static_fn to_static_array() {
  return {};
}
} // namespace rflect
