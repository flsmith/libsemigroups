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

#include "constants.hpp"
#include "libsemigroups-debug.hpp"
#include "order.hpp"
#include "string.hpp"

namespace libsemigroups {

  template <typename PlusOp,
            typename ProdOp,
            typename ZeroOp,
            typename OneOp,
            size_t N,
            typename Container>
  class Matrix {
   public:
    using scalar_type    = typename Container::value_type;
    using container_type = Container;

    using Plus = PlusOp;
    using Prod = ProdOp;
    using Zero = ZeroOp;
    using One  = OneOp;

    Matrix() = default;

    explicit Matrix(Container&& c) : _container(std::move(c)) {
      LIBSEMIGROUPS_ASSERT(_container.size() == N * N);
    }

    explicit Matrix(std::initializer_list<std::initializer_list<scalar_type>> C)
        : Matrix() {
      // TODO checks
      for (size_t r = 0; r < N; ++r) {
        for (size_t c = 0; c < N; ++c) {
          _container[r * N + c] = *((C.begin() + r)->begin() + c);
        }
      }
    }
    // TODO the other constructors

    bool operator==(Matrix const& other) const {
      return _container == other._container;
    }

    bool operator!=(Matrix const& other) const {
      return _container != other._container;
    }

    void operator()(size_t r, size_t c, scalar_type val) {
      _container[r * N + c] = val;
    }

    typename Container::const_iterator operator[](size_t r) {
      return _container.cbegin() + (r * N);
    }

    //! Returns \c true if \c this is less than \p that.
    bool operator<(Matrix const& that) const {
      return this->_container < that._container;
    }

    //! Insertion operator
    // TODO formatting
    friend std::ostringstream& operator<<(std::ostringstream& os,
                                          Matrix const&       x) {
      os << x._container;
      return os;
    }

    //! Insertion operator
    friend std::ostream& operator<<(std::ostream& os, Matrix const& x) {
      os << detail::to_string(x);
      return os;
    }

    void product_inplace(Matrix const& A, Matrix const& B) {
      // get a pointer array for the entire column in B matrix
      static std::array<scalar_type, N> colPtr;
      // loop over output columns first, because column element addresses are
      // not continuous
      for (size_t c = 0; c < N; c++) {
        for (size_t i = 0; i < N; ++i) {
          colPtr[i] = B._container[i * N + c];
        }
        for (size_t r = 0; r < N; r++) {
          _container[r * N + c]
              = std::inner_product(A._container.begin() + r * N,
                                   A._container.begin() + (r + 1) * N,
                                   colPtr.cbegin(),
                                   ZeroOp()(),
                                   Plus(),
                                   Prod());
        }
      }
    }

    Matrix operator*(Matrix const& y) const {
      Matrix xy;
      xy.product_inplace(*this, y);
      return xy;
    }

    // REMOVE
    size_t degree() const {
      return N;
    }

    static Matrix identity() {
      Container x;
      x.fill(ZeroOp()());
      for (size_t i = 0; i < N; ++i) {
        x[i * N + i] = OneOp()();
      }
      return Matrix(std::move(x));
    }

    // TODO this should perhaps take the semiring into account
    // REMOVE
    size_t complexity() const {
      return N * N * N;
    }

    // TODO cache and shouldn't be std::hash
    // REMOVE
    size_t hash_value() const {
      return std::hash<Container>()(_container);
    }

    Matrix transpose() const {
      Container x;
      x.fill(ZeroOp()());
      for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
          x[i * N + j] = _container[j * N + i];
        }
      }
      return Matrix(std::move(x));
    }

    // Acts on the rows by this, and stores the result in res.
    void
    right_product(std::vector<std::array<scalar_type, N>>&       res,
                  std::vector<std::array<scalar_type, N>> const& rows) const {
      // TODO assertions
      // TODO duplication
      res.resize(rows.size());
      static std::array<scalar_type, N> colPtr;
      for (size_t c = 0; c < N; c++) {
        for (size_t i = 0; i < N; ++i) {
          colPtr[i] = _container[i * N + c];
        }
        for (size_t r = 0; r < rows.size(); r++) {
          res[r][c] = std::inner_product(rows[r].begin(),
                                         rows[r].begin() + N,
                                         colPtr.cbegin(),
                                         ZeroOp()(),
                                         Plus(),
                                         Prod());
        }
      }
    }

    class Row;

    class Rows {
     public:
      using const_iterator = typename Container::const_iterator;
      using iterator       = typename Container::iterator;
      Rows() : Rows(new Container(), true) {}
      explicit Rows(Matrix const& m) : Rows(&m._container, false) {}

      ~Rows() {
        if (_owns_data) {
          delete _data;
        }
      }

      Row const& operator[](size_t i) const {
        return _rows[i];
      }

      const_iterator cbegin() const {
        return _rows.cbegin();
      }

      const_iterator cend() const {
        return _rows.cend();
      }

      void push_back(const_iterator first) {
        LIBSEMIGROUPS_ASSERT(size() < N);
        acquire_data();
        for (size_t i = 0; i < N; ++i) {
          (*_data)[N * size() + i] = first[i];
        }
        renew_iterators();  // TODO move this into acquire_data
        _rows.emplace_back(end_data());
      }

      void pop_back() {
        _rows.pop_back();
      }

      void push_back(Row const& r) {
        push_back(r.cbegin());
      }

      void resize(size_t M) {
        LIBSEMIGROUPS_ASSERT(M <= N);
        if (M > size()) {
          auto first = _data->begin() + size() * N;
          auto last  = first + (M - size()) * N;
          for (auto it = first; it < last; it += N) {
            _rows.emplace_back(it);
          }
        }
      }

      void clear() {
        _rows.clear();
      }

      Row const& back() const {
        return _rows.back();
      }

      Row& back() {
        return _rows.back();
      }

      // Eg sort(LexicographicCompare());
      template <typename F>
      void sort(F&& func) {
        std::sort(_rows.begin(), _rows.end(), [&func](Row& r1, Row& r2) {
          return func(r1.cbegin(), r1.cend(), r2.cbegin(), r2.cend());
        });
      }

      size_t size() const noexcept {
        return _rows.size();
      }

     private:
      explicit Rows(Container const* C, bool owns)
          : _data(const_cast<Container*>(C)), _owns_data(owns), _rows() {
        if (!_owns_data) {
          for (auto it = _data->begin(); it < _data->end(); it += N) {
            _rows.emplace_back(it);
          }
        }
      }

      iterator end_data() {
        return _data->begin() + size() * N;
      }

      void acquire_data() {
        if (!_owns_data) {
          _data      = new Container(*_data);
          _owns_data = true;
        }
      }

      void renew_iterators() {
        LIBSEMIGROUPS_ASSERT(_owns_data);
        auto it = _data->begin();
        for (size_t i = 0; i < _rows.size(); ++i) {
          _rows[i].renew_iterator(it);
          it += N;
        }
      }

      // _data has fixed size N * N,
      Container*                    _data;
      bool                          _owns_data;
      detail::StaticVector1<Row, N> _rows;
    };

    class Row {
      friend class Rows;

     public:
      using const_iterator = typename Container::const_iterator;
      using iterator       = typename Container::iterator;

      Row() = default;  // caution
      explicit Row(iterator first) : _begin(first) {}

      Row(Row const&) = default;
      Row(Row&&)      = default;
      Row& operator=(Row const&) = default;
      Row& operator=(Row&&) = default;

      scalar_type const& operator[](size_t i) const {
        return _begin[i];
      }

      scalar_type& operator[](size_t i) {
        return _begin[i];
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

      void operator+=(Row const& x) {
        auto& this_ = *this;
        for (size_t i = 0; i < N; ++i) {
          this_[i] = Plus()(this_[i], x[i]);
        }
      }

      void operator+=(scalar_type a) {
        auto& this_ = *this;
        for (auto& x : *this) {
          x = Plus()(x, a);
        }
      }

      void operator*=(scalar_type a) {
        auto& this_ = *this;
        for (size_t i = 0; i < N; ++i) {
          this_[i] = Prod()(this_[i], a);
        }
      }

      bool operator==(Row const& that) const {
        return std::equal(_begin, _begin + N, that._begin);
      }

      bool operator!=(Row const& that) const {
        return !(*this == that);
      }

      bool operator<(Row const& that) const {
        return std::lexicographical_compare(
            cbegin(), cend(), that.cbegin(), that.cend());
      }

     private:
      void renew_iterator(iterator first) {
        _begin = first;
      }

      iterator _begin;
    };

    static_assert(std::is_trivial<Row>(), "Row is not a trivial class!");

    Rows rows() const {
      return Rows(*this);
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

  template <size_t N, size_t M>
  using PMat = Matrix<PlusMod<M>,
                      ProdMod<M>,
                      IntegerZero,
                      IntegerOne,
                      N,
                      std::array<uint_fast8_t, N * N>>;

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

  template <size_t N>
  struct MaxPlusProd {
    int64_t operator()(int64_t const x, int64_t const y) const {
      if (x == NEGATIVE_INFINITY || y == NEGATIVE_INFINITY) {
        return NEGATIVE_INFINITY;
      }
      return std::min(x + y, static_cast<int64_t>(N));
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

  template <size_t dim, size_t thresh>
  using TropicalMaxPlusMat = Matrix<MaxPlusPlus,
                                    MaxPlusProd<thresh>,
                                    MaxPlusZero,
                                    MaxPlusOne,
                                    dim,
                                    std::array<int64_t, dim * dim>>;

  namespace matrix_helpers {
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
      // TODO static assert
      Container out(row);
      for (size_t i = 0; i < out.size(); ++i) {
        out[i] = Prod()(out[i], scalar);
      }
      return out;
    }

    template <size_t dim, size_t thresh>
    typename TropicalMaxPlusMat<dim, thresh>::Rows
    row_basis(TropicalMaxPlusMat<dim, thresh> const& x) {
      using matrix_type = TropicalMaxPlusMat<dim, thresh>;
      using scalar_type = typename matrix_type::scalar_type;
      using Rows        = typename matrix_type::Rows;
      using Zero        = typename matrix_type::Zero;
      using Plus        = typename matrix_type::Plus;

      Rows result;
      auto rows = x.rows();
      rows.sort(LexicographicalCompare<typename matrix_type::container_type>());
      for (size_t r1 = 0; r1 < rows.size(); ++r1) {
        if (r1 == 0 || rows[r1] != rows[r1 - 1]) {
          result.resize(result.size() + 1);
          std::fill(result.back().begin(), result.back().end(), Zero()());
          for (size_t r2 = 0; r2 < r1; ++r2) {
            scalar_type max_scalar = thresh;
            for (size_t c = 0; c < dim; ++c) {
              if (rows[r2][c] == Zero()()) {
                continue;
              }
              if (rows[r1][c] >= rows[r2][c]) {
                if (rows[r1][c] != thresh) {
                  max_scalar = Plus()(max_scalar, rows[r1][c] - rows[r2][c]);
                }
              } else {
                max_scalar = Zero()();
                break;
              }
            }
            if (max_scalar != Zero()()) {
              result.back() += rows[r2];
              // multiply every entry by max_scalar
              result.back() *= max_scalar;
            }
          }
          if (result.back() != rows[r1]) {
            result.pop_back();
            // REWORK this so that we don't actually copy rows[r1]
            result.push_back(rows[r1]);
            if (result.size() == dim) {
              break;
            }
          } else {
            result.pop_back();
          }
        }
      }
      return result;
    }

    template <size_t dim, size_t thresh>
    void
    tropical_max_plus_row_basis(std::vector<std::array<int64_t, dim>>& rows) {
      //! Modifies \p res to contain the row space basis of \p x.
      // TODO assertions, proper types
      static thread_local std::vector<std::array<int64_t, dim>> buf;
      buf.clear();
      std::sort(rows.begin(), rows.end());
      for (size_t row = 0; row < rows.size(); ++row) {
        // TODO fix this def
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
  }  // namespace matrix_helpers

}  // namespace libsemigroups

#endif  // LIBSEMIGROUPS_MATRIX_HPP_
