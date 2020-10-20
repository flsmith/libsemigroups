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

#ifndef LIBSEMIGROUPS_MATRIX_HPP_
#define LIBSEMIGROUPS_MATRIX_HPP_

#include <algorithm>  // for min

#include "adapters.hpp"
#include "constants.hpp"
#include "libsemigroups-debug.hpp"
#include "order.hpp"
#include "string.hpp"

namespace libsemigroups {
  // Forward decl
  template <typename PlusOp,
            typename ProdOp,
            typename ZeroOp,
            typename OneOp,
            size_t R,
            size_t C,
            typename Container>
  class Matrix;

  // Forward decl
  template <typename PlusOp,
            typename ProdOp,
            typename ZeroOp,
            typename OneOp,
            size_t N,
            typename Container>
  using Row = Matrix<PlusOp, ProdOp, ZeroOp, OneOp, 1, N, Container>;

  // Class for cheaply storing iterators to rows
  template <typename PlusOp,
            typename ProdOp,
            typename ZeroOp,
            typename OneOp,
            size_t N,
            typename Container>
  class RowView {
   public:
    using const_iterator = typename Container::const_iterator;
    using iterator       = typename Container::iterator;
    using scalar_type    = typename Container::value_type;

    using Row = Row<PlusOp, ProdOp, ZeroOp, OneOp, N, Container>;

    RowView() = default;
    explicit RowView(iterator first) : _begin(first) {}
    explicit RowView(const_iterator first)
        : _begin(const_cast<iterator>(first)) {}
    explicit RowView(Row const& r) : _begin(const_cast<Row&>(r).begin()) {}

    RowView(RowView const&) = default;
    RowView(RowView&&)      = default;
    RowView& operator=(RowView const&) = default;
    RowView& operator=(RowView&&) = default;

    scalar_type const& operator[](size_t i) const {
      return _begin[i];
    }

    scalar_type& operator[](size_t i) {
      return _begin[i];
    }

    scalar_type const& operator()(size_t i) const {
      return (*this)[i];
    }

    scalar_type& operator()(size_t i) {
      return (*this)[i];
    }

    const_iterator cbegin() const {
      return _begin;
    }

    const_iterator cend() const {
      return _begin + N;
    }

    const_iterator begin() const {
      return _begin;
    }

    const_iterator end() const {
      return _begin + N;
    }

    iterator begin() {
      return _begin;
    }

    iterator end() {
      return _begin + N;
    }

    void operator+=(RowView const& x) {
      auto& this_ = *this;
      for (size_t i = 0; i < N; ++i) {
        this_[i] = PlusOp()(this_[i], x[i]);
      }
    }

    void operator+=(scalar_type const a) {
      for (auto& x : *this) {
        x = PlusOp()(x, a);
      }
    }

    void operator*=(scalar_type const a) {
      for (auto& x : *this) {
        x = ProdOp()(x, a);
      }
    }

    Row operator*(scalar_type a) const {
      Row result(*this);
      result *= a;
      return result;
    }

    template <typename T>
    bool operator==(T const& that) const {
      return std::equal(begin(), end(), that.begin());
    }

    template <typename T,
              typename
              = typename std::enable_if<std::is_same<T, RowView>::value
                                        || std::is_same<T, Row>::value>::type>
    bool operator!=(T const& that) const {
      return !(*this == that);
    }

    template <typename T,
              typename
              = typename std::enable_if<std::is_same<T, RowView>::value
                                        || std::is_same<T, Row>::value>::type>
    bool operator<(T const& that) const {
      return std::lexicographical_compare(
          cbegin(), cend(), that.cbegin(), that.cend());
    }

   private:
    iterator _begin;
  };

  struct MatrixPolymorphicBase {};

  template <typename PlusOp,
            typename ProdOp,
            typename ZeroOp,
            typename OneOp,
            size_t R,
            size_t C,
            typename Container>
  class Matrix : MatrixPolymorphicBase {
   public:
    using scalar_type    = typename Container::value_type;
    using container_type = Container;
    using iterator       = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;

    using Plus = PlusOp;
    using Prod = ProdOp;
    using Zero = ZeroOp;
    using One  = OneOp;
    // FIXME Container in the next two lines has the wrong size if it's an
    // array, because Container = std::array<scalar_type, R * C> and it should
    // be std::array<scalar_type, C> in the following two aliases.
    using RowContainer = typename std::conditional<
        std::is_same<std::array<scalar_type, R * C>, Container>::value,
        std::array<scalar_type, C>,
        Container>::type;
    using Row     = Row<PlusOp, ProdOp, ZeroOp, OneOp, C, RowContainer>;
    using RowView = RowView<PlusOp, ProdOp, ZeroOp, OneOp, C, RowContainer>;

    static_assert(std::is_trivial<RowView>(),
                  "RowView is not a trivial class!");

    ////////////////////////////////////////////////////////////////////////
    // Constructors + destructor
    ////////////////////////////////////////////////////////////////////////

    Matrix() = default;

    explicit Matrix(RowView const& rv) : _container() {
      std::copy(rv.cbegin(), rv.cend(), begin());
      static_assert(
          R == 1, "Cannot construct matrix with more than 1 row from RowView");
    }

    explicit Matrix(Container&& c) : _container(std::move(c)) {
      LIBSEMIGROUPS_ASSERT(_container.size() == R * C);
    }

    explicit Matrix(Container const& c) : _container(c) {
      LIBSEMIGROUPS_ASSERT(_container.size() == R * C);
    }

    explicit Matrix(std::initializer_list<std::initializer_list<scalar_type>> m)
        : Matrix() {
      LIBSEMIGROUPS_ASSERT(m.size() == R);
      LIBSEMIGROUPS_ASSERT(std::for_each(
          m.begin(), m.end(), [](std::initializer_list<scalar_type> const& r) {
            return r.size() == C;
          }));
      for (size_t r = 0; r < R; ++r) {
        auto row = m.begin() + r;
        for (size_t c = 0; c < C; ++c) {
          _container[r * C + c] = *(row->begin() + c);
        }
      }
    }

    Matrix(Matrix const&) = default;
    Matrix(Matrix&&)      = default;
    Matrix& operator=(Matrix const&) = default;
    Matrix& operator=(Matrix&&) = default;

    Matrix& operator=(RowView const& rv) {
      static_assert(
          R == 1, "Cannot construct matrix with more than 1 row from RowView");
      std::copy(rv.cbegin(), rv.cend(), begin());
      return *this;
    }

    ~Matrix() = default;

    ////////////////////////////////////////////////////////////////////////
    // Comparison operators
    ////////////////////////////////////////////////////////////////////////

    bool operator==(Matrix const& other) const {
      return _container == other._container;
    }

    bool operator!=(Matrix const& other) const {
      return _container != other._container;
    }

    bool operator<(Matrix const& that) const {
      return this->_container < that._container;
    }

    ////////////////////////////////////////////////////////////////////////
    // Comparison with RowView operators
    ////////////////////////////////////////////////////////////////////////

    bool operator==(RowView const& other) const {
      static_assert(R == 1, "Cannot compare non-row matrices and RowView!");
      return static_cast<RowView>(*this) == other;
    }

    bool operator!=(RowView const& other) const {
      static_assert(R == 1, "Cannot compare non-row matrices and RowView!");
      return static_cast<RowView>(*this) != other;
    }

    bool operator<(RowView const& other) const {
      static_assert(R == 1, "Cannot compare non-row matrices and RowView!");
      return static_cast<RowView>(*this) < other;
    }

    ////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////

    scalar_type& operator()(size_t r, size_t c) {
      return _container[r * C + c];
    }

    scalar_type const& operator()(size_t r, size_t c) const {
      return _container[r * C + c];
    }

    static constexpr size_t number_of_rows() noexcept {
      return R;
    }

    static constexpr size_t number_of_cols() noexcept {
      return C;
    }

    size_t hash_value() const {
      return Hash<Container>()(_container);
    }

    ////////////////////////////////////////////////////////////////////////
    // Arithmetic operators - in-place
    ////////////////////////////////////////////////////////////////////////

    void product_inplace(Matrix const& A, Matrix const& B) {
      static_assert(R == C, "can only multiply square matrices");
      Matrix* BB = &const_cast<Matrix&>(B);
      if (&A == &B) {
        BB = new Matrix(B);
      }
      BB->transpose();

      LIBSEMIGROUPS_ASSERT(&A != &BB);

      for (size_t c = 0; c < C; c++) {
        for (size_t r = 0; r < R; r++) {
          _container[r * C + c]
              = std::inner_product(A._container.begin() + r * C,
                                   A._container.begin() + (r + 1) * C,
                                   BB->_container.begin() + (c * R),
                                   Zero()(),
                                   Plus(),
                                   Prod());
        }
      }
      if (&A != &B) {
        const_cast<Matrix&>(B).transpose();
      } else {
        delete BB;
      }
      LIBSEMIGROUPS_ASSERT(B == BB);
    }

    void transpose() {
      static_assert(C == R, "cannot transpose non-square matrix");
      auto& x = *this;
      for (size_t r = 0; r < R - 1; ++r) {
        for (size_t c = r + 1; c < C; ++c) {
          std::swap(x(r, c), x(c, r));
        }
      }
    }

    void operator+=(Matrix const& that) {
      auto& this_ = *this;
      for (size_t r = 0; r < R; ++r) {
        for (size_t c = 0; c < C; ++c) {
          this_(r, c) = Plus()(this_(r, c), that(r, c));
        }
      }
    }

    void operator*=(scalar_type const a) {
      for (auto& x : *this) {
        x = ProdOp()(x, a);
      }
    }

    void operator+=(RowView const& that) {
      static_assert(R == 1,
                    "cannot add RowView to matrix with more than one row");
      RowView(*this) += that;
    }

    ////////////////////////////////////////////////////////////////////////
    // Arithmetic operators - not in-place
    ////////////////////////////////////////////////////////////////////////

    Matrix operator+(Matrix const& y) const {
      Matrix result(*this);
      result += y;
      return result;
    }

    Matrix operator*(Matrix const& y) const {
      Matrix xy;
      xy.product_inplace(*this, y);
      return xy;
    }

    static Matrix identity() {
      static_assert(C == R, "cannot create non-square identity matrix");
      Matrix x;
      x.fill(ZeroOp()());
      for (size_t r = 0; r < R; ++r) {
        x(r, r) = OneOp()();
      }
      return x;
    }

    // Acts on the rows by this, and stores the result in res.
    // TODO: replace
    // void
    // right_product(std::vector<std::array<scalar_type, N>>&       res,
    //               std::vector<std::array<scalar_type, N>> const& rows)
    //               const
    //               {
    //   // TODO assertions
    //   // TODO duplication
    //   res.resize(rows.size());
    //   static std::array<scalar_type, N> colPtr;
    //   for (size_t c = 0; c < N; c++) {
    //     for (size_t i = 0; i < N; ++i) {
    //       colPtr[i] = _container[i * N + c];
    //     }
    //     for (size_t r = 0; r < rows.size(); r++) {
    //       res[r][c] = std::inner_product(rows[r].begin(),
    //                                      rows[r].begin() + N,
    //                                      colPtr.cbegin(),
    //                                      ZeroOp()(),
    //                                      Plus(),
    //                                      Prod());
    //     }
    //   }
    // }

    ////////////////////////////////////////////////////////////////////////
    // Iterators
    ////////////////////////////////////////////////////////////////////////

    iterator begin() {
      static_assert(R == 1, "this must be a row");
      return _container.begin();
    }

    iterator end() {
      static_assert(R == 1, "this must be a row");
      return _container.end();
    }

    const_iterator begin() const {
      static_assert(R == 1, "this must be a row");
      return _container.begin();
    }

    const_iterator end() const {
      static_assert(R == 1, "this must be a row");
      return _container.end();
    }

    const_iterator cbegin() const {
      static_assert(R == 1, "this must be a row");
      return _container.cbegin();
    }

    const_iterator cend() const {
      static_assert(R == 1, "this must be a row");
      return _container.cend();
    }

    ////////////////////////////////////////////////////////////////////////
    // Rows
    ////////////////////////////////////////////////////////////////////////

    // TODO template for StaticVector1 too
    void rows(std::vector<RowView>& x) const {
      for (auto it = _container.begin(); it < _container.end(); it += C) {
        x.emplace_back(it);
      }
    }

    RowView row(size_t i) const {
      LIBSEMIGROUPS_ASSERT(i < R);
      return RowView(_container.cbegin() + i * C);
    }

    void rows(std::array<RowView, R>& x) const {
      auto it = const_cast<typename Container::iterator>(_container.begin());
      for (size_t r = 0; r < R; ++r) {
        x[r] = RowView(it);
        it += C;
      }
    }

    void fill(scalar_type const& val) {
      std::fill(_container.begin(), _container.end(), val);
    }

    ////////////////////////////////////////////////////////////////////////
    // Conversion operators - rows only
    ////////////////////////////////////////////////////////////////////////

    //! Insertion operator
    // TODO formatting
    friend std::ostringstream& operator<<(std::ostringstream& os,
                                          Matrix const&       x) {
      if (R == 1) {
        os << x._container;
      }
      return os;
    }

    //! Insertion operator
    friend std::ostream& operator<<(std::ostream& os, Matrix const& x) {
      os << detail::to_string(x);
      return os;
    }

   private:
    Container _container;
  };  // namespace libsemigroups

  struct BooleanPlus {
    bool operator()(bool const x, bool const y) const {
      return x || y;
    }
  };

  struct BooleanProd {
    bool operator()(bool const x, bool const y) const {
      return x && y;
    }
  };

  struct BooleanOne {
    bool operator()() {
      return true;
    }
  };

  struct BooleanZero {
    bool operator()() {
      return false;
    }
  };

  template <size_t N>
  using BMat = Matrix<BooleanPlus,
                      BooleanProd,
                      BooleanZero,
                      BooleanOne,
                      N,
                      N,
                      std::array<bool, N * N>>;

  template <size_t N>
  struct PlusMod {
    uint_fast8_t operator()(uint_fast8_t const x, uint_fast8_t const y) const {
      return (x + y) % N;
    }
  };

  template <size_t N>
  struct ProdMod {
    uint_fast8_t operator()(uint_fast8_t const x, uint_fast8_t const y) const {
      return (x * y) % N;
    }
  };

  struct IntegerOne {
    uint_fast8_t operator()() {
      return 1;
    }
  };

  struct IntegerZero {
    uint_fast8_t operator()() {
      return 0;
    }
  };

  template <size_t R, size_t C, size_t M>
  using PMat = Matrix<PlusMod<M>,
                      ProdMod<M>,
                      IntegerZero,
                      IntegerOne,
                      R,
                      C,
                      std::array<uint_fast8_t, R * C>>;

  struct MaxPlusPlus {
    int64_t operator()(int64_t const x, int64_t const y) const {
      if (x == NEGATIVE_INFINITY) {
        return y;
      } else if (y == NEGATIVE_INFINITY) {
        return x;
      }
      return std::max(x, y);
    }
  };

  template <size_t T>
  struct MaxPlusProd {
    int64_t operator()(int64_t const x, int64_t const y) const {
      if (x == NEGATIVE_INFINITY || y == NEGATIVE_INFINITY) {
        return NEGATIVE_INFINITY;
      }
      return std::min(x + y, static_cast<int64_t>(T));
    }
  };

  struct MaxPlusOne {
    int64_t operator()() {
      return 0;
    }
  };

  struct MaxPlusZero {
    int64_t operator()() {
      return NEGATIVE_INFINITY;
    }
  };

  template <size_t R, size_t C, size_t T>
  using TropicalMaxPlusMat = Matrix<MaxPlusPlus,
                                    MaxPlusProd<T>,
                                    MaxPlusZero,
                                    MaxPlusOne,
                                    R,
                                    C,
                                    std::array<int64_t, R * C>>;

  namespace matrix_helpers {
    template <typename T,
              typename S = std::array<typename T::RowView, T::number_of_rows()>>
    S rows(T const& x) {
      S container;
      x.rows(container);
      return container;
    }

    template <size_t R, size_t C, size_t T, typename Container>
    void row_basis(TropicalMaxPlusMat<R, C, T> const& x, Container& result) {
      using matrix_type = TropicalMaxPlusMat<R, C, T>;
      using scalar_type = typename matrix_type::scalar_type;
      using RowView     = typename matrix_type::RowView;
      using Row         = typename matrix_type::Row;
      using Zero        = typename matrix_type::Zero;

      auto views = rows(x);
      std::sort(views.begin(),
                views.end(),
                [](RowView const& rv1, RowView const& rv2) {
                  return std::lexicographical_compare(
                      rv1.begin(), rv1.end(), rv2.begin(), rv2.end());
                });
      Row tmp1;

      for (size_t r1 = 0; r1 < views.size(); ++r1) {
        if (r1 == 0 || views[r1] != views[r1 - 1]) {
          std::fill(tmp1.begin(), tmp1.end(), Zero()());
          for (size_t r2 = 0; r2 < r1; ++r2) {
            scalar_type max_scalar = T;
            for (size_t c = 0; c < C; ++c) {
              if (views[r2][c] == Zero()()) {
                continue;
              }
              if (views[r1][c] >= views[r2][c]) {
                if (views[r1][c] != T) {
                  max_scalar
                      = std::min(max_scalar, views[r1][c] - views[r2][c]);
                }
              } else {
                max_scalar = Zero()();
                break;
              }
            }
            if (max_scalar != Zero()()) {
              tmp1 += views[r2] * max_scalar;
            }
          }
          if (tmp1 != views[r1]) {
            result.push_back(views[r1]);
          }
        }
      }
    }

    template <size_t R,
              size_t C,
              size_t T,
              typename Container = detail::StaticVector1<
                  typename TropicalMaxPlusMat<R, C, T>::RowView,
                  R>>
    Container row_basis(TropicalMaxPlusMat<R, C, T> const& x) {
      Container result;
      row_basis(x, result);
      return result;
    }

  }  // namespace matrix_helpers

  ////////////////////////////////////////////////////////////////////////
  // Adapters
  ////////////////////////////////////////////////////////////////////////

  template <typename T>
  struct Complexity<
      T,
      typename std::enable_if<std::is_base_of<MatrixPolymorphicBase, T>::value,
                              void>::type> {
    constexpr size_t operator()(T const& x) const {
      return x.number_of_rows() * x.number_of_rows() * x.number_of_rows();
    }
  };

  template <typename T>
  struct Degree<
      T,
      typename std::enable_if<std::is_base_of<MatrixPolymorphicBase, T>::value,
                              void>::type> {
    constexpr size_t operator()(T const& x) const {
      return x.number_of_rows();
    }
  };

  template <typename T>
  struct Hash<
      T,
      typename std::enable_if<std::is_base_of<MatrixPolymorphicBase, T>::value,
                              void>::type> {
    constexpr size_t operator()(T const& x) const {
      return x.hash_value();
    }
  };

  template <typename T>
  struct IncreaseDegree<
      T,
      typename std::enable_if<std::is_base_of<MatrixPolymorphicBase, T>::value,
                              void>::type> {
    void operator()(T&, size_t) const {
      // static_assert(false, "Cannot increase degree for Matrix");
      LIBSEMIGROUPS_ASSERT(false);
    }
  };

  template <typename T>
  struct One<
      T,
      typename std::enable_if<std::is_base_of<MatrixPolymorphicBase, T>::value,
                              void>::type> {
    inline T operator()(T const& x) const {
      return x.identity();
    }
  };

  template <typename T>
  struct Product<
      T,
      typename std::enable_if<std::is_base_of<MatrixPolymorphicBase, T>::value,
                              void>::type> {
    inline void operator()(T& xy, T const& x, T const& y, size_t = 0) const {
      xy.product_inplace(x, y);
    }
  };

}  // namespace libsemigroups

#endif  // LIBSEMIGROUPS_MATRIX_HPP_