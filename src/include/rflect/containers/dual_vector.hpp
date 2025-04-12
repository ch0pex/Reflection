/************************************************************************
 * Copyright (c) 2025 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file dual_vector.hpp
 * @version 1.0
 * @date 14/03/2025
 * @brief Short description
 *
 * Longer description
 */

#pragma once

#include <rflect/concepts/layout_concepts.hpp>
#include <rflect/concepts/proxy_concepts.hpp>
#include <rflect/containers/iterator.hpp>
#include <rflect/converters/to_static.hpp>
#include <rflect/introspection/struct.hpp>

#include <iostream>
#include <ranges>

namespace rflect {

template<has_proxy T, memory_layout Layout = layout::aos, template<typename> class Alloc = std::allocator>
class dual_vector {
public:
  // ********** Type traits **********
  using value_type           = T;
  using underlying_container = typename Layout::template vector<T, Alloc>;
  using view_type            = typename T::template proxy_type<dual_vector>;
  using const_view_type      = typename T::template proxy_type<dual_vector const>;
  using memory_layout        = Layout;
  using iterator             = proxy_iterator<view_type>;
  using const_iterator       = proxy_iterator<const_view_type>;

  // ********** Constructors **********
  constexpr dual_vector() = default;

  constexpr dual_vector(std::initializer_list<value_type> init)
    requires(soa_layout<memory_layout>)
  {
    for (auto const& item: init) {
      push_back(item);
    }
  }

  constexpr dual_vector(std::initializer_list<value_type> init)
    requires(aos_layout<memory_layout>)
    : data_(init) { }

  constexpr dual_vector(std::integral auto size)
    requires(aos_layout<memory_layout>)
    : data_(size) { }

  constexpr explicit dual_vector(std::integral auto size)
    requires(soa_layout<memory_layout>)
  {
    template for (constexpr auto member: nonstatic_data_members_of(^^underlying_container) | to_static_array) {
      data_.[:member:] = decltype(data_.[:member:])(size);
    }
  }


  // ********** Member functions **********

  constexpr view_type at(std::size_t const index) { return {data_, index}; }

  constexpr const_view_type at(std::size_t const index) const { return const_view_type {data_, index}; }

  constexpr view_type operator[](std::size_t const index) { return {data_, index}; }

  constexpr const_view_type operator[](std::size_t const index) const { return {data_, index}; }

  constexpr view_type front() { return {data_, 0}; }

  constexpr view_type back() { return {data_, size() - 1}; }

  constexpr auto begin() { return iterator {data_, 0}; }

  constexpr auto end() { return iterator {data_, size()}; }

  constexpr auto begin() const { return const_iterator {data_, 0}; }

  constexpr auto end() const { return const_iterator {data_, size()}; }

  // ********** SOA member functions **********

  constexpr void push_back(value_type const& item)
    requires(soa_layout<memory_layout>)
  {
    template for (constexpr auto member: nonstatic_data_members_of(^^underlying_container) | to_static_array) {
      data_.[:member:].push_back(item.[:nonstatic_data_member<value_type>(identifier_of(member)):]);
    }
  }

  constexpr void push_back(view_type const value)
    requires(soa_layout<memory_layout>)
  {

    auto tuple          = *value;
    constexpr auto size = (nonstatic_data_members_of(^^underlying_container) | to_static_array).size();
    template for (constexpr auto index: static_iota<size>()) {
      data_.[:nonstatic_data_member<underlying_container>([:index:]):].push_back(std::get<[:index:]>(tuple));
    }
  }

  [[nodiscard]] constexpr std::size_t size() const
    requires(soa_layout<memory_layout>)
  {
    return data_.[:nonstatic_data_member<underlying_container>(0):].size();
  }

  // ********** AOS member functions **********

  constexpr void push_back(value_type const& item)
    requires(aos_layout<memory_layout>)
  {
    data_.push_back(item);
  }

  constexpr void push_back(view_type const view)
    requires(aos_layout<memory_layout>)
  {
    data_.push_back(*view);
  }

  [[nodiscard]] constexpr std::size_t size() const
    requires(aos_layout<memory_layout>)
  {
    return data_.size();
  }

  // ********** Operators **********

  friend constexpr bool operator==(dual_vector const& vec1, dual_vector const& vec2)
    requires(aos_layout<memory_layout>)
  {
    return vec1.data_ == vec2.data_;
  }

  friend constexpr bool operator==(dual_vector const& vec1, dual_vector const& vec2)
    requires(soa_layout<memory_layout>)
  {
    bool equal = true;
    template for (constexpr auto member: nonstatic_data_members_of(^^underlying_container) | to_static_array) {
      equal &= (vec1.data_.[:member:] == vec2.data_.[:member:]);
    };
    return equal;
  }

private:
  underlying_container data_;
};

// --- Deducing guidelines  ---
template<has_proxy T>
dual_vector(std::initializer_list<T> init) -> dual_vector<T>;


} // namespace rflect
