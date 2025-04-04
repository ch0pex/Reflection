/************************************************************************
 * Copyright (c) 2025 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file proxy_class.hpp
 * @version 1.0
 * @date 28/03/2025
 * @brief Short description
 *
 * Longer description
 */

#pragma once

#include "utils.hpp"

#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...) __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...) macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS(macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

#define EXPAND_PROXY_METHOD(name, ...)                                                                                 \
  auto name() -> decltype(auto) {                                                                                      \
    using namespace acb;                                                                                               \
    return (this->template member<#name##_ss>());                                                                      \
  }                                                                                                                    \
  auto name() const -> decltype(auto) {                                                                                \
    using namespace acb;                                                                                               \
    return (this->template member<#name##_ss>());                                                                      \
  }

#define DEFINE_PROXY_METHODS(name, ...)                                                                                \
  using acb::ProxyBase<name, Container>::operator=;                                                                    \
  using acb::ProxyBase<name, Container>::ProxyBase;                                                                    \
  FOR_EACH(EXPAND_PROXY_METHOD, __VA_ARGS__)

#define DEFINE_PROXY(...)                                                                                              \
  template<typename Container>                                                                                         \
  struct proxy_type : acb::ProxyBase<proxy_type, Container> {                                                          \
    DEFINE_PROXY_METHODS(proxy_type, __VA_ARGS__)                                                                      \
  }

namespace acb {

namespace detail {
template<typename T>
consteval auto type_soa_zipview() {

  constexpr auto raw_members = define_static_array(
      nonstatic_data_members_of(^^T) | std::views::transform(std::meta::type_of) |
      std::views::transform(std::meta::remove_cvref) | std::ranges::to<std::vector>()
  );


  std::vector<std::meta::info> value_members;

  template for (constexpr auto member: raw_members) {
    auto new_member = add_lvalue_reference(^^typename[:member:] ::value_type const);
    // auto const_member = add_const(new_member);
    value_members.push_back(new_member);
  }

  return substitute(^^std::tuple, value_members);
}

template<typename To, typename From, std::meta::info... members>
constexpr auto soa_to_zip_helper(From const& from, std::size_t const index) -> To {

  auto value = std::views::zip(from.[:members:]...);
  return value[index];
}

template<typename From>
consteval auto get_soa_to_zip_helper() {
  using To = [:type_soa_zipview<From>():];

  std::vector args = {^^To, ^^From};
  for (auto mem: nonstatic_data_members_of(^^From)) {
    args.push_back(reflect_value(mem));
  }

  return extract<To (*)(From const&, std::size_t const index)>(substitute(^^soa_to_zip_helper, args));
}

template<typename From>
constexpr auto soa_to_zip(From const& from, std::size_t const index) {
  return get_soa_to_zip_helper<From>()(from, index);
}

} // namespace detail

template<template<typename> class Proxy, class Container>
class ProxyBase {
public:
  // *** Type traits ***
  using container            = Container;
  using proxy_type           = Proxy<container>;
  using value_type           = typename Container::value_type;
  using underlying_container = std::conditional_t<
      std::is_const_v<container>, typename Container::underlying_container const,
      typename Container::underlying_container>;

  // *** Constructors ***
  ProxyBase(underlying_container& cont, std::size_t const index) : index_(index), container_(cont) { }

  // *** Operators ***
  constexpr proxy_type& operator=(value_type const& value)
    requires(aos_layout<container>)
  {
    container_.at(index_) = value;
    return *this;
  }

  constexpr proxy_type& operator=(value_type const& value)
    requires(soa_layout<container>)
  {
    template for (constexpr auto member: data_member_array(^^underlying_container)) {
      container_.[:member:].at(index_) = value.[:nonstatic_data_member<value_type>(identifier_of(member)):];
    }
    return *this;
  }

  template<typename Self>
  constexpr value_type& operator*(this Self&& self)
    requires(aos_layout<container>)
  {
    return self.container_.at(self.index_);
  }

  template<typename Self>
  constexpr auto operator*(this Self&& self)
    requires(soa_layout<container>)
  {
    return soa_to_zip(self.container_, self.index_);
  }

protected:
  template<char const* name, typename Self>
    requires(aos_layout<container>)
  constexpr auto member(this Self&& self) -> decltype(auto) {
    return (self.container_.at(self.index_).[:nonstatic_data_member<value_type>(name):]);
  }

  template<char const* name, typename Self>
    requires(soa_layout<container>)
  constexpr auto member(this Self&& self) -> decltype(auto) {
    return (self.container_.[:nonstatic_data_member<underlying_container>(name):].at(self.index_));
  }

private:
  std::size_t index_;
  underlying_container& container_;
};


} // namespace acb
