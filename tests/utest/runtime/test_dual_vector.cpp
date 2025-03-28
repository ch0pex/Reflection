/************************************************************************
 * Copyright (c) 2025 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_dual_vector.cpp
 * @version 1.0
 * @date 28/03/2025
 * @brief Short description
 *
 * Longer description
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "reflected/reflection/dual_vector.hpp"
#include "reflected/reflection/proxy_class.hpp"

using namespace acb;

struct Mock {
  DEFINE_PROXY(id, density, velocity);

  std::int32_t id;
  std::double_t density;
  std::array<std::double_t, 3> velocity;
};

constexpr bool operator==(Mock const& a, Mock const& b) {
  return a.id == b.id and a.density == b.density and a.velocity == b.velocity;
}

template<typename C>
constexpr bool operator==(Mock::proxy_type<C> a, Mock const& b) {
  return a.id() == b.id and a.density() == b.density and a.velocity() == b.velocity;
}

template<typename C>
constexpr bool operator==(Mock const& a, Mock::proxy_type<C> const& b) {
  return b == a;
}

static_assert(has_proxy<Mock>);

constexpr Mock mock_0 {.id = 0, .density = 12.15, .velocity = {1.0, 2.0, 3.0}};

constexpr Mock mock_1 {.id = 1, .density = 13.15, .velocity = {0.0}};

constexpr Mock mock_2 {.id = 2, .density = -132.0, .velocity = {1.0, 2.0, 3.0}};

constexpr Mock mock_3 {.id = 3, .density = -132.0, .velocity = {1.0, 2.0, 3.0}};

TEST_SUITE_BEGIN("Dual Vector");

TEST_CASE_TEMPLATE("Constructor with initializer list", T, layout::aos, layout::soa) {
  dual_vector<Mock, T> mock_vector {mock_0, mock_1, mock_2};
  dual_vector<Mock, T> mock_vector_2;
  mock_vector_2.push_back(mock_0);
  mock_vector_2.push_back(mock_1);
  mock_vector_2.push_back(mock_2);

  CHECK(mock_vector == mock_vector_2);
}

TEST_CASE_TEMPLATE("push_back", T, layout::aos, layout::soa) { dual_vector<Mock, T> mock; }

TEST_CASE_TEMPLATE("at", T, layout::aos, layout::soa) {
  dual_vector<Mock, T> mock {mock_0, mock_1, mock_2};

  CHECK(mock.at(0) == mock_0);
  CHECK(mock.at(1) == mock_1);
  CHECK(mock.at(2) == mock_2);
}

TEST_CASE_TEMPLATE("operator[]", T, layout::aos, layout::soa) {
  dual_vector<Mock, T> mock {mock_0, mock_1, mock_2};

  CHECK(mock[0] == mock_0);
  CHECK(mock[1] == mock_1);
  CHECK(mock[2] == mock_2);
}

TEST_CASE_TEMPLATE("size", T, layout::aos, layout::soa) {
  dual_vector<Mock, T> vec_0;
  dual_vector<Mock, T> vec_1 {mock_0, mock_1};
  dual_vector<Mock, T> vec_2 {mock_0, mock_1, mock_2};

  CHECK(vec_0.size() == 0U);
  CHECK(vec_1.size() == 2U);
  CHECK(vec_2.size() == 3U);
}

TEST_CASE_TEMPLATE("emplace_back", T, layout::aos, layout::soa) { CHECK(false); }

TEST_CASE_TEMPLATE("range iteration", T, layout::aos, layout::soa) { CHECK(false); }

TEST_SUITE_END();
