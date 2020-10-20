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
  template <typename Plus, typename Container>
  struct RowAddition {
    void operator()(Container& x, Container const& y) const {
      LIBSEMIGROUPS_ASSERT(x.size() == y.size());
      for (size_t i = 0; i < x.size(); ++i) {
        x[i] = Plus()(x[i], y[i]);
      }
    }

    void operator()(Container&       res,
                    Container const& x,
                    Container const& y) const {
      LIBSEMIGROUPS_ASSERT(res.size() == x.size());
      LIBSEMIGROUPS_ASSERT(x.size() == y.size());
      for (size_t i = 0; i < x.size(); ++i) {
        res[i] = Plus()(x[i], y[i]);
      }
    }
  };

  template <typename Prod, typename Container>
  Container scalar_row_product(Container                      row,
                               typename Container::value_type scalar) {
    Container out(row);
    for (size_t i = 0; i < out.size(); ++i) {
      out[i] = Prod()(out[i], scalar);
    }
    return out;
  }

  template <size_t dim, size_t thresh>
  void
  tropical_max_plus_row_basis(std::vector<std::array<int64_t, dim>>& rows) {
    static thread_local std::vector<std::array<int64_t, dim>> buf;
    buf.clear();
    std::sort(rows.begin(), rows.end());
    for (size_t row = 0; row < rows.size(); ++row) {
      std::array<int64_t, dim> sum;
      sum.fill(NEGATIVE_INFINITY);
      if (row == 0 || rows[row] != rows[row - 1]) {
        for (size_t row2 = 0; row2 < row; ++row2) {
          int64_t max_scalar = thresh;
          for (size_t col = 0; col < dim; ++col) {
            if (rows[row2][col] == NEGATIVE_INFINITY) {
              continue;
            }
            if (rows[row][col] >= rows[row2][col]) {
              if (rows[row][col] != thresh) {
                max_scalar
                    = std::min(max_scalar, rows[row][col] - rows[row2][col]);
              }
            } else {
              max_scalar = NEGATIVE_INFINITY;
              break;
            }
          }
          if (max_scalar != NEGATIVE_INFINITY) {
            auto scalar_prod = scalar_row_product<MaxPlusProd<thresh>,
                                                  std::array<int64_t, dim>>(
                rows[row2], max_scalar);
            RowAddition<MaxPlusPlus, std::array<int64_t, dim>>()(sum,
                                                                 scalar_prod);
          }
        }
        if (sum != rows[row]) {
          buf.push_back(rows[row]);
        }
      }
    }
    std::swap(buf, rows);
  }

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
    auto          rg = ReportGuard(REPORT);
    PMat<3, 3, 3> m;
    m.product_inplace(PMat<3, 3, 3>({{1, 1, 0}, {0, 0, 1}, {1, 0, 1}}),
                      PMat<3, 3, 3>({{1, 0, 1}, {0, 0, 1}, {1, 1, 0}}));
    REQUIRE(m == PMat<3, 3, 3>({{1, 0, 2}, {1, 1, 0}, {2, 1, 1}}));
  }

  LIBSEMIGROUPS_TEST_CASE("Matrix", "004", "rows", "[quick]") {
    auto    rg = ReportGuard(REPORT);
    BMat<2> m({1, 1, 0, 0});
    using RowView = typename BMat<2>::RowView;
    auto r        = matrix_helpers::rows(m);
    REQUIRE(std::vector<bool>(r[0].cbegin(), r[0].cend())
            == std::vector<bool>({true, true}));
    REQUIRE(std::vector<bool>(r[1].cbegin(), r[1].cend())
            == std::vector<bool>({false, false}));
    REQUIRE(r.size() == 2);
    std::sort(r.begin(), r.end(), [](RowView const& rv1, RowView const& rv2) {
      return std::lexicographical_compare(
          rv1.begin(), rv1.end(), rv2.begin(), rv2.end());
    });
    REQUIRE(std::vector<bool>(r[0].cbegin(), r[0].cend())
            == std::vector<bool>({false, false}));
    REQUIRE(std::vector<bool>(r[1].cbegin(), r[1].cend())
            == std::vector<bool>({true, true}));
  }

  LIBSEMIGROUPS_TEST_CASE("TropicalMaxPlusMat", "005", "row space", "[quick]") {
    using Mat         = TropicalMaxPlusMat<2, 2, 5>;
    using scalar_type = typename Mat::scalar_type;
    {
      Mat m1;
      m1.fill(NEGATIVE_INFINITY);
      REQUIRE(m1
              == Mat({{NEGATIVE_INFINITY, NEGATIVE_INFINITY},
                      {NEGATIVE_INFINITY, NEGATIVE_INFINITY}}));
      Mat m2;
      m2.fill(4);
      REQUIRE(m1 + m2 == m2);
      REQUIRE(m2(0, 1) == 4);
    }

    auto rg = ReportGuard(REPORT);
    {
      std::vector<std::array<int64_t, 2>> expected;
      expected.push_back({1, 1});
      expected.push_back({0, 0});
      tropical_max_plus_row_basis<2, 5>(expected);
      REQUIRE(expected.size() == 1);
      REQUIRE(expected.at(0) == std::array<int64_t, 2>({0, 0}));

      Mat  m({1, 1, 0, 0});
      auto r = matrix_helpers::row_basis(m);
      REQUIRE(r.size() == 1);
      REQUIRE(std::vector<scalar_type>(r[0].cbegin(), r[0].cend())
              == std::vector<scalar_type>({0, 0}));
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

  LIBSEMIGROUPS_TEST_CASE("PMat", "006", "row view", "[quick]") {
    using Mat         = PMat<4, 4, 10>;
    using Row         = typename Mat::Row;
    using RowView     = typename Mat::RowView;
    using scalar_type = typename Mat::scalar_type;

    auto rg = ReportGuard(REPORT);
    Mat  m({{1, 1, 0, 0}, {2, 0, 2, 0}, {1, 2, 3, 9}, {0, 0, 0, 7}});
    auto r = matrix_helpers::rows(m);
    REQUIRE(r.size() == 4);
    REQUIRE(std::vector<scalar_type>(r[0].cbegin(), r[0].cend())
            == std::vector<scalar_type>({1, 1, 0, 0}));
    r[0] += r[1];
    REQUIRE(std::vector<scalar_type>(r[0].cbegin(), r[0].cend())
            == std::vector<scalar_type>({3, 1, 2, 0}));
    REQUIRE(std::vector<scalar_type>(r[1].cbegin(), r[1].cend())
            == std::vector<scalar_type>({2, 0, 2, 0}));
    REQUIRE(m == Mat({{3, 1, 2, 0}, {2, 0, 2, 0}, {1, 2, 3, 9}, {0, 0, 0, 7}}));
    REQUIRE(r[0][0] == 3);
    REQUIRE(r[0](0) == 3);
    REQUIRE(r[2](3) == 9);
    std::sort(r[0].begin(), r[0].end());
    REQUIRE(std::vector<scalar_type>(r[0].cbegin(), r[0].cend())
            == std::vector<scalar_type>({0, 1, 2, 3}));
    REQUIRE(m == Mat({{0, 1, 2, 3}, {2, 0, 2, 0}, {1, 2, 3, 9}, {0, 0, 0, 7}}));
    r[0] += 11;
    REQUIRE(std::vector<scalar_type>(r[0].cbegin(), r[0].cend())
            == std::vector<scalar_type>({1, 2, 3, 4}));
    REQUIRE(m == Mat({{1, 2, 3, 4}, {2, 0, 2, 0}, {1, 2, 3, 9}, {0, 0, 0, 7}}));
    r[1] *= 3;
    REQUIRE(m == Mat({{1, 2, 3, 4}, {6, 0, 6, 0}, {1, 2, 3, 9}, {0, 0, 0, 7}}));
    REQUIRE(std::vector<scalar_type>(r[1].cbegin(), r[1].cend())
            == std::vector<scalar_type>({6, 0, 6, 0}));
    REQUIRE(r[2] < r[1]);
    r[1] = r[2];
    REQUIRE(m == Mat({{1, 2, 3, 4}, {6, 0, 6, 0}, {1, 2, 3, 9}, {0, 0, 0, 7}}));
    REQUIRE(r[1] == r[2]);
    REQUIRE(r[1] == Row({{1, 2, 3, 9}}));

    RowView rv;
    {
      rv = r[0];
      REQUIRE(rv == r[0]);
      REQUIRE(&rv != &r[0]);
    }
  }

  LIBSEMIGROUPS_TEST_CASE("PMat", "007", "row view vs row", "[quick]") {
    using Mat = PMat<4, 4, 10>;
    using Row = typename Mat::Row;

    auto rg = ReportGuard(REPORT);
    Mat  m({{1, 1, 0, 0}, {2, 0, 2, 0}, {1, 2, 3, 9}, {0, 0, 0, 7}});
    auto r = matrix_helpers::rows(m);
    REQUIRE(r.size() == 4);
    REQUIRE(r[0] == Row({{1, 1, 0, 0}}));
    REQUIRE(r[1] == Row({{2, 0, 2, 0}}));
    REQUIRE(r[0] != Row({{2, 0, 2, 0}}));
    REQUIRE(r[1] != Row({{1, 1, 0, 0}}));
    REQUIRE(Row({{1, 1, 0, 0}}) == r[0]);
    REQUIRE(Row({{2, 0, 2, 0}}) == r[1]);
    REQUIRE(Row({{2, 0, 2, 0}}) != r[0]);
    REQUIRE(Row({{1, 1, 0, 0}}) != r[1]);
    REQUIRE(Row({{1, 1, 0, 0}}) < Row({{9, 9, 9, 9}}));
    REQUIRE(r[0] < Row({{9, 9, 9, 9}}));
    REQUIRE(!(Row({{9, 9, 9, 9}}) < r[0]));
    Row x(r[3]);
    x *= 3;
    REQUIRE(x == Row({{0, 0, 0, 1}}));
    REQUIRE(r[3] == Row({{0, 0, 0, 7}}));
    REQUIRE(r[3] != x);
    REQUIRE(x != r[3]);
    REQUIRE(!(x != x));
  }

  LIBSEMIGROUPS_TEST_CASE("TropicalMaxPlusMat", "008", "row space", "[quick]") {
    using Mat = TropicalMaxPlusMat<4, 4, 5>;
    using Row = typename Mat::Row;

    Mat  m({{2, 2, 0, 1},
           {0, 0, 1, 3},
           {1, NEGATIVE_INFINITY, 0, 0},
           {0, 1, 0, 1}});
    auto rg = ReportGuard(REPORT);
    auto r  = matrix_helpers::row_basis(m);
    REQUIRE(r.size() == 4);
    REQUIRE(r[0] == Row({0, 0, 1, 3}));
    REQUIRE(r[1] == Row({0, 1, 0, 1}));
    REQUIRE(r[2] == Row({1, NEGATIVE_INFINITY, 0, 0}));
    REQUIRE(r[3] == Row({2, 2, 0, 1}));

    std::vector<std::array<int64_t, 4>> expected;
    expected.push_back({2, 2, 0, 1});
    expected.push_back({0, 0, 1, 3});
    expected.push_back({1, NEGATIVE_INFINITY, 0, 0});
    expected.push_back({0, 1, 0, 1});
    tropical_max_plus_row_basis<4, 5>(expected);
    REQUIRE(expected.size() == 4);
  }

}  // namespace libsemigroups
