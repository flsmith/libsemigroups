//
// libsemigroups - C++ library for semigroups and monoids
// Copyright (C) 2020 James D. Mitchell
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

#include "catch.hpp"                           // for REQUIRE
#include "libsemigroups/cache.hpp"             // for Cache
#include "libsemigroups/element-adapters.hpp"  // for Product
#include "libsemigroups/element.hpp"           // for Transformation
#include "test-main.hpp"                       // for LIBSEMIGROUPS_TEST_CASE

namespace libsemigroups {
  namespace detail {
    LIBSEMIGROUPS_TEST_CASE("Cache",
                            "000",
                            "initial",
                            "[quick][transformation]") {
      Cache<Transformation<size_t>*> cache;
      REQUIRE(cache.acquirable() == 0);
      REQUIRE(cache.acquired() == 0);
      REQUIRE_THROWS_AS(cache.acquire(), LibsemigroupsException);
      Transformation<size_t> t({0, 1, 3, 2});
      cache.push(&t, 5);
      REQUIRE(cache.acquirable() == 5);
      REQUIRE(cache.acquired() == 0);
      Transformation<size_t>& x = *cache.acquire();
      REQUIRE(cache.acquirable() == 4);
      REQUIRE(cache.acquired() == 1);
      x.increase_degree_by(10);
      cache.release(&x);
      REQUIRE(cache.acquired() == 0);
      REQUIRE(cache.acquirable() == 5);
    }

    LIBSEMIGROUPS_TEST_CASE("Cache",
                            "001",
                            "boolean matrices",
                            "[quick][boolmat]") {
      Cache<BooleanMat*> cache;
      REQUIRE(cache.acquirable() == 0);
      REQUIRE(cache.acquired() == 0);
      REQUIRE_THROWS_AS(cache.acquire(), LibsemigroupsException);
      BooleanMat* b = new BooleanMat({{0, 1, 0}, {1, 1, 1}, {0, 0, 1}});
      cache.push(b, 1);
      REQUIRE(cache.acquirable() == 1);
      REQUIRE(cache.acquired() == 0);
      BooleanMat* tmp = cache.acquire();
      REQUIRE(cache.acquirable() == 0);
      REQUIRE(cache.acquired() == 1);
      REQUIRE_THROWS_AS(cache.acquire(), LibsemigroupsException);
      cache.release(tmp);
      REQUIRE(cache.acquirable() == 1);
      REQUIRE(cache.acquired() == 0);
      delete b;
    }

    LIBSEMIGROUPS_TEST_CASE("Cache", "002", "CacheGuard", "[quick][boolmat]") {
      Cache<BooleanMat*> cache;
      REQUIRE(cache.acquirable() == 0);
      REQUIRE(cache.acquired() == 0);
      REQUIRE_THROWS_AS(cache.acquire(), LibsemigroupsException);
      BooleanMat* b = new BooleanMat({{0, 1, 0}, {1, 1, 1}, {0, 0, 1}});
      cache.push(b, 2);
      REQUIRE(cache.acquirable() == 2);
      REQUIRE(cache.acquired() == 0);
      {
        CacheGuard<BooleanMat*> cg1(cache);
        REQUIRE(cache.acquirable() == 1);
        REQUIRE(cache.acquired() == 1);
        BooleanMat* tmp1 = cg1.get();
        REQUIRE(b != tmp1);
        {
          CacheGuard<BooleanMat*> cg2(cache);
          BooleanMat*             tmp2 = cg2.get();
          REQUIRE(cache.acquirable() == 0);
          REQUIRE(cache.acquired() == 2);
          REQUIRE_THROWS_AS(cache.acquire(), LibsemigroupsException);
          REQUIRE_THROWS_AS(CacheGuard<BooleanMat*>(cache),
                            LibsemigroupsException);
          REQUIRE(tmp1 != tmp2);
          REQUIRE(b != tmp2);
        }
        REQUIRE(cache.acquirable() == 1);
        REQUIRE(cache.acquired() == 1);
      }
      REQUIRE(cache.acquirable() == 2);
      REQUIRE(cache.acquired() == 0);
      REQUIRE(cache.acquired() == 0);
      REQUIRE(cache.acquirable() == 2);
      delete b;
    }

    LIBSEMIGROUPS_TEST_CASE("Cache",
                            "003",
                            "transformation products",
                            "[quick][transformation]") {
      Cache<Transformation<size_t>*> cache;
      Transformation<size_t>         t({0, 1, 3, 2, 5, 7, 3, 4});
      cache.push(&t, 5);
      Transformation<size_t>* x = cache.acquire();
      Transformation<size_t>* y = cache.acquire();
      REQUIRE(x != y);
      REQUIRE(&t != x);
      REQUIRE(&t != y);
      *y = t;
      Product<Transformation<size_t>*>()(x, &t, y);
      REQUIRE(*x == t * t);
      REQUIRE(&t != x);
      REQUIRE(&t != y);
      Product<Transformation<size_t>*>()(y, &t, x);
      REQUIRE(*y == t * t * t);
    }
  }  // namespace detail
}  // namespace libsemigroups