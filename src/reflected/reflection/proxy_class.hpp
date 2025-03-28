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

template<template<typename> class Proxy, class Container>
class ProxyBase {
public:
  // *** Type traits ***
  using container            = Container;
  using proxy_type           = Proxy<container>;
  using value_type           = typename Container::value_type;
  using underlying_container = typename Container::underlying_container;

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
    template for (constexpr auto member: data_member_array(^^typename Container::underlying_container)) {
      container_.[:member:].at(index_) = value.[:member_named<value_type>(identifier_of(member)):];
    }
    return *this;
  }

  constexpr value_type& operator*()
    requires(aos_layout<container>)
  {
    return container_.at(index_);
  }

  constexpr value_type const& operator*() const
    requires(aos_layout<container>)
  {
    return container_.at(index_);
  }

protected:
  template<char const* name>
    requires(aos_layout<container>)
  constexpr auto member() -> decltype(auto) {
    return (container_.at(index_).[:member_named<value_type>(name):]);
  }

  template<char const* name>
    requires(soa_layout<container>)
  constexpr auto member() -> decltype(auto) {
    return (container_.[:member_named<underlying_container>(name):].at(index_));
  }

  template<char const* name>
    requires(aos_layout<container>)
  constexpr auto member() const -> decltype(auto) {
    return (container_.at(index_).[:member_named<value_type>(name):]);
  }

  template<char const* name>
    requires(soa_layout<container>)
  constexpr auto member() const -> decltype(auto) {
    return (container_.[:member_named<underlying_container>(name):].at(index_));
  }

private:
  std::size_t index_;
  underlying_container& container_;

  template<typename U>
  friend class ProxyIterator;
};

template<typename T>
class ProxyIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type   = std::size_t;
  using value_type        = T;
  using reference         = T&;
  using pointer           = T*;

  ProxyIterator(typename T::underlying_container& cont, std::size_t const index) : value_(cont, index) { }

  explicit ProxyIterator(value_type const& value) : value_(value) { }

  constexpr ProxyIterator operator++() {
    ++value_.index_;
    return ProxyIterator {value_};
  }

  constexpr ProxyIterator operator++(int) {
    ProxyIterator old = *this;
    operator++();
    return old;
  }

  constexpr reference operator*() { return value_; }

  constexpr reference operator->() { return &value_; }

  friend constexpr bool operator==(ProxyIterator const& proxy1, ProxyIterator const& proxy2) {
    return proxy1.value_.index_ == proxy2.value_.index_ and
           std::addressof(proxy1.value_.container_) == std::addressof(proxy2.value_.container_);
  }

  friend constexpr bool operator!=(ProxyIterator const& proxy1, ProxyIterator const& proxy2) {
    return not(proxy1 == proxy2);
  }

private:
  value_type value_;
};

} // namespace acb
