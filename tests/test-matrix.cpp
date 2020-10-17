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

#include <array>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint64_t

#include "catch.hpp"  // for REQUIRE, REQUIRE_THROWS_AS, REQUIRE_NOTHROW
#include "libsemigroups/containers.hpp"  // for ?
#include "libsemigroups/matrix.hpp"      // for ?
#include "libsemigroups/order.hpp"       // for ?
#include "libsemigroups/report.hpp"      // for ?
#include "test-main.hpp"                 // for LIBSEMIGROUPS_TEST_CASE

namespace libsemigroups {
  constexpr bool REPORT = false;

  LIBSEMIGROUPS_TEST_CASE("Matrix", "001", "", "[quick]") {
    auto    rg = ReportGuard(REPORT);
    BMat<2> m({0, 1, 0, 1});
    REQUIRE(m == BMat<2>({0, 1, 0, 1}));
    REQUIRE(!(m == BMat<2>({0, 0, 0, 1})));
    REQUIRE(m == BMat<2>({{0, 1}, {0, 1}}));
    m.product_inplace(BMat<2>({{0, 0}, {0, 0}}), BMat<2>({{0, 0}, {0, 0}}));
    REQUIRE(m == BMat<2>({{0, 0}, {0, 0}}));
    m.product_inplace(BMat<2>({{0, 0}, {0, 0}}), BMat<2>({{1, 1}, {1, 1}}));
    REQUIRE(m == BMat<2>({{0, 0}, {0, 0}}));
    m.product_inplace(BMat<2>({{1, 1}, {1, 1}}), BMat<2>({{0, 0}, {0, 0}}));
    REQUIRE(m == BMat<2>({{0, 0}, {0, 0}}));

    m.product_inplace(BMat<2>({{0, 1}, {1, 0}}), BMat<2>({{1, 0}, {1, 0}}));
    REQUIRE(m == BMat<2>({{1, 0}, {1, 0}}));
  }

  LIBSEMIGROUPS_TEST_CASE("Matrix", "002", "", "[quick]") {
    auto    rg = ReportGuard(REPORT);
    BMat<3> m;
    m.product_inplace(BMat<3>({{1, 1, 0}, {0, 0, 1}, {1, 0, 1}}),
                      BMat<3>({{1, 0, 1}, {0, 0, 1}, {1, 1, 0}}));
    REQUIRE(m == BMat<3>({{1, 0, 1}, {1, 1, 0}, {1, 1, 1}}));
  }

  LIBSEMIGROUPS_TEST_CASE("Matrix", "003", "", "[quick]") {
    auto       rg = ReportGuard(REPORT);
    PMat<3, 3> m;
    m.product_inplace(PMat<3, 3>({{1, 1, 0}, {0, 0, 1}, {1, 0, 1}}),
                      PMat<3, 3>({{1, 0, 1}, {0, 0, 1}, {1, 1, 0}}));
    REQUIRE(m == PMat<3, 3>({{1, 0, 2}, {1, 1, 0}, {2, 1, 1}}));
  }

  LIBSEMIGROUPS_TEST_CASE("Matrix", "004", "rows", "[quick]") {
    auto    rg = ReportGuard(REPORT);
    BMat<2> m({1, 1, 0, 0});
    auto    r = m.rows();
    REQUIRE(std::vector<bool>(r[0].cbegin(), r[0].cend())
            == std::vector<bool>({true, true}));
    REQUIRE(std::vector<bool>(r[1].cbegin(), r[1].cend())
            == std::vector<bool>({false, false}));
    REQUIRE(r.size() == 2);
    r.sort(LexicographicalCompare<std::vector<bool>>());
    REQUIRE(std::vector<bool>(r[0].cbegin(), r[0].cend())
            == std::vector<bool>({false, false}));
    REQUIRE(std::vector<bool>(r[1].cbegin(), r[1].cend())
            == std::vector<bool>({true, true}));
  }

  LIBSEMIGROUPS_TEST_CASE("TropicalMaxPlusMat", "005", "row space", "[quick]") {
    using Mat         = TropicalMaxPlusMat<2, 5>;
    using scalar_type = typename Mat::scalar_type;

    auto rg = ReportGuard(REPORT);
    {
      Mat  m({1, 1, 0, 0});
      auto r = matrix_helpers::row_basis(m);
      REQUIRE(r.size() == 1);
      REQUIRE(std::vector<scalar_type>(r[0].cbegin(), r[0].cend())
              == std::vector<scalar_type>({0, 0}));

      std::vector<std::array<int64_t, 2>> expected;
      expected.push_back({1, 1});
      expected.push_back({0, 0});
      matrix_helpers::tropical_max_plus_row_basis<2, 5>(expected);
      REQUIRE(expected.size() == 1);
      REQUIRE(expected.at(0) == std::array<int64_t, 2>({0, 0}));
    }
    {
      auto m = Mat::identity();
      auto r = matrix_helpers::row_basis(m);
      REQUIRE(r.size() == 2);
      REQUIRE(std::vector<scalar_type>(r[0].cbegin(), r[0].cend())
              == std::vector<scalar_type>({NEGATIVE_INFINITY, 0}));
      REQUIRE(std::vector<scalar_type>(r[1].cbegin(), r[1].cend())
              == std::vector<scalar_type>({0, NEGATIVE_INFINITY}));
    }
  }

}  // namespace libsemigroups
