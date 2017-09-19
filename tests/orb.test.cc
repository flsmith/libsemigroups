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

TEST_CASE("Orb 01: ??", "[quick][orb][01]") {
  std::vector<Element*> gens = {new Permutation<u_int16_t>({1, 0, 2}),
                                new Permutation<u_int16_t>({1, 2, 0})};
  Semigroup S = Semigroup(gens);
  S.set_report(SEMIGROUPS_REPORT);
  really_delete_cont(gens);

  REQUIRE(S.size() == 2);
  REQUIRE(S.degree() == 3);
  REQUIRE(S.nridempotents() == 2);
  REQUIRE(S.nrgens() == 2);
  REQUIRE(S.nrrules() == 4);

  Element* expected = new Transformation<u_int16_t>({0, 1, 0});
  REQUIRE(*S[0] == *expected);
  expected->really_delete();
  delete expected;

  expected = new Transformation<u_int16_t>({0, 1, 2});
  REQUIRE(*S[1] == *expected);
  expected->really_delete();
  delete expected;

  Element* x = new Transformation<u_int16_t>({0, 1, 0});
  REQUIRE(S.position(x) == 0);
  REQUIRE(S.test_membership(x));
  x->really_delete();
  delete x;

  x = new Transformation<u_int16_t>({0, 1, 2});
  REQUIRE(S.position(x) == 1);
  REQUIRE(S.test_membership(x));
  x->really_delete();
  delete x;

  x = new Transformation<u_int16_t>({0, 0, 0});
  REQUIRE(S.position(x) == Semigroup::UNDEFINED);
  REQUIRE(!S.test_membership(x));
  x->really_delete();
  delete x;
}
