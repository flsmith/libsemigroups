//
// libsemigroups - C++ library for semigroups and monoids
// Copyright (C) 2017 James D. Mitchell
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../src/orb.h"
#include "catch.hpp"

#define ORB_REPORT false

using namespace libsemigroups;

template <typename T> struct VectorEqual {
  bool operator()(std::vector<T>* pt1, std::vector<T>* pt2) const {
    return *pt1 == *pt2;
  }
};

template <typename T> struct VectorHash {
  size_t operator()(std::vector<T> const* vec) const {
    size_t seed = 0;
    for (auto const& x : *vec) {
      seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

// FIXME fails for small values of n
template <class TElementType>
std::vector<TElementType*>* symmetric_group(size_t n) {
  std::vector<u_int16_t> a = {1, 0}, b;
  for (size_t i = 2; i < n; ++i) {
    a.push_back(i);
  }
  for (size_t i = 1; i < n; ++i) {
    b.push_back(i);
  }
  b.push_back(0);
  return new std::vector<TElementType*>(
      {new TElementType(a), new TElementType(b)});
}

// FIXME fails for small values of n
std::vector<Transformation<u_int16_t>*>* full_trans_monoid(size_t n) {
  std::vector<Transformation<u_int16_t>*>* gens
      = symmetric_group<Transformation<u_int16_t>>(n);

  std::vector<u_int16_t> c = {0};
  for (size_t i = 0; i < n - 1; ++i) {
    c.push_back(i);
  }
  gens->push_back(new Transformation<u_int16_t>(c));
  return gens;
}

TEST_CASE("Orb 01: symmetric group 3 on points", "[quick][orb][01]") {
  std::vector<Permutation<u_int16_t>*> gens
      = {new Permutation<u_int16_t>({1, 0, 2}),
         new Permutation<u_int16_t>({1, 2, 0})};

  auto act = [](
      Permutation<u_int16_t>* perm, u_int16_t pt, u_int16_t tmp) -> u_int16_t {
    (void) tmp;
    return (*perm)[pt];
  };

  Orb<Permutation<u_int16_t>, u_int16_t> o(gens, 0, act);
  REQUIRE(o.size() == 3);
}

TEST_CASE("Orb 02: symmetric group 20 on 4-tuples", "[quick][orb][02]") {
  typedef Permutation<u_int16_t> element_type;
  typedef std::vector<u_int16_t> point_type;
  typedef Orb<element_type,
              point_type*,
              VectorHash<u_int16_t>,
              VectorEqual<u_int16_t>>
      orbit_type;

  auto act
      = [](element_type* perm, point_type* pt, point_type* tmp) -> point_type* {
    tmp->clear();
    tmp->reserve(pt->size());
    for (size_t i = 0; i < pt->size(); ++i) {
      tmp->push_back((*perm)[(*pt)[i]]);
    }
    return tmp;
  };

  auto copier
      = [](point_type* pt) -> point_type* { return new point_type(*pt); };

  point_type*                 seed = new point_type({0, 1, 2, 3});
  std::vector<element_type*>* gens
      = symmetric_group<Permutation<u_int16_t>>(20);
  orbit_type o(*gens, seed, act, copier);
  o.reserve(116280);

  REQUIRE(o.size() == 116280);
  REQUIRE(o.position(seed) == 0);
  REQUIRE(*o[0] == *seed);
  REQUIRE(*o.at(0) == *seed);

  {
    point_type* pt = new point_type({9, 0, 2, 19});
    REQUIRE(o.position(pt) != orbit_type::UNDEFINED);
    REQUIRE(o.position(pt) == 25295);
    REQUIRE(*o[25295] == *pt);
    REQUIRE(*o.at(25295) == *pt);
    delete pt;
  }

  {
    point_type*   pt          = new point_type({9, 0, 2, 19});
    element_type* mapper      = o.mapper(1);
    point_type*   tmp_element = new point_type({0, 0, 0, 0});
    REQUIRE(*o.at(1) == *act(mapper, seed, tmp_element));
    mapper = o.mapper(25295);
    REQUIRE(*act(mapper, seed, tmp_element) == *pt);
    delete mapper;
    delete tmp_element;
  }

  {
    point_type* pt = new point_type({0});
    REQUIRE(o.position(pt) == orbit_type::UNDEFINED);
    delete pt;
  }
}

TEST_CASE("Orb 03: full transformation monoid lvalue orbit",
          "[quick][orb][03]") {
  typedef Transformation<u_int16_t> element_type;
  typedef std::vector<u_int16_t>    point_type;
  typedef Orb<element_type,
              point_type*,
              VectorHash<u_int16_t>,
              VectorEqual<u_int16_t>>
      orbit_type;

  auto act
      = [](element_type* t, point_type* pt, point_type* tmp) -> point_type* {
    return t->lvalue(pt, tmp);
  };

  auto copier
      = [](point_type* pt) -> point_type* { return new point_type(*pt); };

  orbit_type o(*full_trans_monoid(23),
               new point_type({0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                               12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22}),
               act,
               copier);
  o.reserve(std::pow(2, 24));

  REQUIRE(o.size() == std::pow(2, 23) - 1);
}
