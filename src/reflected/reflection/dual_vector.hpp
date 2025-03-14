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

#include <experimental/meta>
#include <array>
#include <vector>
#include <algorithm>
#include <ranges>
#include <print>
#include <functional>
#include <cassert>
#include <iostream>

/// *** Implementation, this will be needed till expansions are fully implemented
namespace __impl {
  template<auto... vals>
  struct replicator_type {
    template<typename F>
      constexpr void operator>>(F body) const {
        (body.template operator()<vals>(), ...);
      }
  };

  template<auto... vals>
  replicator_type<vals...> replicator = {};
}

template<typename R>
consteval auto expand(R range) {
  std::vector<std::meta::info> args;
  for (auto r : range) {
    args.push_back(reflect_value(r));
  }
  return substitute(^^__impl::replicator, args);
}

/************ This will work fine when expansion statements are finished
    constexpr view at(std::size_t index) {
        view value;
        template for (constexpr std::meta::info member : nonstatic_data_members_of(^view)) {
            value.[:member:] = &data_.[:member_named<container>(identifier_of(member)):];
        }
        return value;
    }

    constexpr void push_back(T const& item) {
        template for (constexpr std::meta::info member : nonstatic_data_members_of(^container)) {
            data_.[:member:].push_back(item.[:member_named<T>(identifier_of(member)):]);
        }
    }
    */

/// *** Meanwhile will have to use this workaround ***
#define template_for(elem_name, elements)  [:expand(elements):] >> [&]<auto elem_name>

/// *** Utility ***


template<typename T>
consteval auto member_number(std::size_t number) {
  return std::meta::nonstatic_data_members_of(^^T)[number];
}

template<typename T>
consteval auto member_named(std::string_view name) {
  for (std::meta::info field : nonstatic_data_members_of(^^T)) {
    if (has_identifier(field) && identifier_of(field) == name)
      return field;
  }
  return std::meta::info(); // This may return something but don't know
}

consteval auto operator""_ss(const char* str, size_t len) -> const char *  {
    return std::meta::define_static_string(str);
}

/// *** Implementation structures ***

template <typename T, size_t N>
struct struct_of_arrays_impl {
  struct impl;

  consteval {
    std::vector<std::meta::info> old_members = nonstatic_data_members_of(^^T);
    std::vector<std::meta::info> new_members = {};
    for (std::meta::info member : old_members) {
        auto array_type = substitute(^^std::array, { type_of(member), std::meta::reflect_value(N)});
        auto mem_descr = data_member_spec(array_type, {.name = identifier_of(member)});
        new_members.push_back(mem_descr);
    }

    define_aggregate(^^impl, new_members);
  }
};

template <class T, template<class> class _Alloc>
struct struct_of_vectors_impl {
  struct impl;

  consteval {
    std::vector<std::meta::info> old_members = nonstatic_data_members_of(^^T);
    std::vector<std::meta::info> new_members = {};

    for (std::meta::info member : old_members) {
        auto allocator = substitute(^^_Alloc, {type_of(member)});
        auto array_type = substitute(^^std::vector, { type_of(member), allocator });
        auto mem_descr = data_member_spec(array_type, {.name = identifier_of(member)});
        new_members.push_back(mem_descr);
    }

    define_aggregate(^^impl, new_members);
  }
};

struct view_impl;

/// *** Memory layouts (AOS, SOA) ***

namespace layout  {
    struct aos {
        template<typename T, std::size_t N>
        using array = std::array<T, N>;

        template<typename T, template<class> class _Alloc>
        using vector = std::vector<T, _Alloc<T>>;
    };

    struct soa {
        template<class T, std::size_t N>
        using array = typename struct_of_arrays_impl<T, N>::impl;

        template<class T, template<class> class _Alloc>
        using vector = struct_of_vectors_impl<T, _Alloc>::impl;
    };
}


/// *** Containers wiht aos/soa layouts
template <class T, size_t N, class _Layout = layout::aos>
using dual_array = _Layout::template array<T,N>;

template <class T, class _Layout = layout::aos, template<typename> class _Alloc = std::allocator>
using dual_vector = _Layout::template vector<T, _Alloc>;

//////////////////////////////////////////////////////////////////////////
/// *** Wrapper that offers the same public interface for soa/aos
//////////////////////////////////////////////////////////////////////////
template<typename T, typename _Layout = layout::aos, template<typename> class _Alloc = std::allocator> /// TODO requires T::View
class DualVector {
public:
    using value_type = T;
    using underlaying_container = dual_vector<T, _Layout, _Alloc>;
    using view = typename T::template proxy_type<DualVector>;
    using memory_layout = _Layout;

    constexpr view at(std::size_t index) {
        return {data_, index};
    }

    constexpr void push_back(value_type const& item) {
        if constexpr (std::same_as<_Layout, layout::soa>) {
            template_for(member, nonstatic_data_members_of(^^underlaying_container)) {
                data_.[:member:].push_back(item.[:member_named<T>(identifier_of(member)):]);
            };
        } else {
           data_.push_back(item);
        }
    }

private:
    underlaying_container data_;
};

template<class Container>
class ProxyBase {

    using value_type = typename Container::value_type;
    using container = typename Container::underlaying_container;

public:
    ProxyBase(typename Container::underlaying_container& cont, std::size_t index) : index_(index), container_(&cont) { }

    constexpr void operator=(value_type const& value) {

        if constexpr(std::same_as<typename Container::memory_layout, layout::aos>) {
            container_->at(index_) = value;
        } else {
            template_for(member, std::meta::nonstatic_data_members_of(^^typename Container::underlaying_container)) {
                container_->[:member:].at(index_) = value.[:member_named<value_type>(identifier_of(member)):];
            };
        }
    }

protected:
    template<const char * name>
    constexpr auto member() -> decltype(auto) {
        if constexpr(std::same_as<typename Container::memory_layout, layout::aos>) {
            assert(index_ < container_->size());
            return (container_->at(index_).[:member_named<value_type>(name):]);
        } else {
            constexpr auto member_name = member_named<container>(name);

            assert(index_ < container_->[:member_name:].size());
            return (container_->[:member_name:].at(index_));
        }
    }

private:
    std::size_t index_;
    container* container_;
};
//////////////////////////////////////////////////////////////////////////



/// *** Actual program not part of the library, user defined ***
//namespace math {
//    template<typename T>
//    struct vec3 {
//       T x,y;
//    };
//}
//
//template<class Container>
//struct ParticleProxy : ProxyBase<Container> {
//
//    // *** Type Traits ***
//    using ProxyBase<Container>::operator=;
//
//
//    // *** Constructors ***
//    ParticleProxy(typename Container::underlaying_container& cont, std::size_t index) : ProxyBase<Container>(cont, index) { }
//
//    /* *** Proxy Methods ***
//     * Define aggregate only supports data_members for now.
//     * It's on the road plan to expand this functionality in a future.
//     * So this proxy methods could be autogenerated in a future.
//     */
//    auto& position() { return this->template member<"position"_ss>(); }
//
//    auto& velocity() { return this->template member<"velocity"_ss>(); }
//
//    auto& acceleration() { return this->template member<"acceleration"_ss>(); }
//
//    auto& hv() { return this->template member<"hv"_ss>(); }
//
//    auto& density() { return this->template member<"density"_ss>(); }
//
//    // *** Actually user defined methods ***
//    void calculateSomeStuff() {
//       position() += velocity() * (1.F/60.F);
//    }
//
//};
//
//struct Particle {
//    template<typename T>
//    using proxy_type = ParticleProxy<T>;
//
//    math::vec3<double> position;
//    math::vec3<double> velocity;
//    math::vec3<double> acceleration;
//    math::vec3<double> hv;
//    double density;
//};
