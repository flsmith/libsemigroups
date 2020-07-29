//
// libsemigroups - C++ library for semigroups and monoids
// Copyright (C) 2019 James D. Mitchell
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

// This file contains the specializations of various adapters for
// derived classes of Element.

#ifndef LIBSEMIGROUPS_ELEMENT_ADAPTERS_HPP_
#define LIBSEMIGROUPS_ELEMENT_ADAPTERS_HPP_

#include <unordered_map>  // for unordered_map
#include <unordered_set>  // for unordered_set

#include "action.hpp"               // for RightAction
#include "adapters.hpp"             // for Complexity etc
#include "bitset.hpp"               // for BitSet
#include "constants.hpp"            // for UNDEFINED
#include "element.hpp"              // for Element
#include "konieczny.hpp"            // for Konieczny
#include "libsemigroups-debug.hpp"  // for LIBSEMIGROUPS_ASSERT
#include "stl.hpp"                  // for Hash, EqualTo, Less

namespace libsemigroups {
  ////////////////////////////////////////////////////////////////////////
  // Complexity specializations
  ////////////////////////////////////////////////////////////////////////

  // Specialization of templates from adapters.hpp for classes derived from
  // the class Element, such as Transformation<size_t> and so on . . .
  //! Specialization of the adapter Complexity for pointers to subclasses of
  //! Element.
  //!
  //! \sa Complexity.
  template <typename TSubclass>
  struct Complexity<TSubclass*,
                    typename std::enable_if<
                        std::is_base_of<Element, TSubclass>::value>::type> {
    //! Returns \p x->complexity().
    inline size_t operator()(TSubclass const* x) const {
      return x->complexity();
    }
  };

  //! Specialization of the adapter Complexity for subclasses of
  //! Element.
  //!
  //! \sa Complexity.
  template <typename TSubclass>
  struct Complexity<TSubclass,
                    typename std::enable_if<
                        std::is_base_of<Element, TSubclass>::value>::type> {
    //! Returns \p x.complexity().
    inline size_t operator()(TSubclass const& x) const {
      return x.complexity();
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // Degree specializations
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter Degree for pointers to subclasses of
  //! Element.
  //!
  //! \sa Degree.
  template <typename TSubclass>
  struct Degree<TSubclass*,
                typename std::enable_if<
                    std::is_base_of<Element, TSubclass>::value>::type> {
    //! Returns \p x->degree().
    inline size_t operator()(TSubclass const* x) const {
      return x->degree();
    }
  };

  //! Specialization of the adapter Degree for subclasses of Element.
  //!
  //! \sa Degree.
  template <typename TSubclass>
  struct Degree<
      TSubclass,
      typename std::enable_if<std::is_base_of<Element, TSubclass>::value,
                              void>::type> {
    //! Returns \p x.degree().
    inline size_t operator()(TSubclass const& x) const {
      return x.degree();
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // IncreaseDegree specializations
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter IncreaseDegree for pointers to subclasses
  //! of Element.
  //!
  //! \sa IncreaseDegree.
  template <typename TSubclass>
  struct IncreaseDegree<TSubclass*,
                        typename std::enable_if<
                            std::is_base_of<Element, TSubclass>::value>::type> {
    //! Returns \p x->increase_degree_by(\p n).
    inline void operator()(TSubclass* x, size_t n) const {
      return x->increase_degree_by(n);
    }
  };

  // TODO(later) why is there no specialisation for non-pointers for
  // IncreaseDegree?

  ////////////////////////////////////////////////////////////////////////
  // Less specializations
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter Less for pointers to subclasses
  //! of Element.
  //!
  //! \sa Less.
  template <typename TSubclass>
  struct Less<TSubclass*,
              typename std::enable_if<
                  std::is_base_of<Element, TSubclass>::value>::type> {
    //! Returns \c true if the object pointed to by \p x is less than the
    //! object pointed to by \p y.
    inline bool operator()(TSubclass const* x, TSubclass const* y) const {
      return *x < *y;
    }
  };

  // TODO(later) why is there no specialisation for non-pointers for
  // Less?

  ////////////////////////////////////////////////////////////////////////
  // One specializations
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter One for pointers to subclasses
  //! of Element.
  //!
  //! \sa One.
  template <typename TSubclass>
  struct One<TSubclass*,
             typename std::enable_if<
                 std::is_base_of<Element, TSubclass>::value>::type> {
    //! Returns a pointer to a new instance of the one of \p x.
    TSubclass* operator()(Element const* x) const {
      // return new TSubclass(std::move(x->identity<TSubclass>()));
      return static_cast<TSubclass*>(x->heap_identity());
    }

    //! Returns a pointer to a new instance of the one of \p x.
    TSubclass* operator()(size_t n) {
      return new TSubclass(std::move(TSubclass::one(n)));
    }
  };

  //! Specialization of the adapter One for subclasses of Element.
  //!
  //! \sa One.
  template <typename TSubclass>
  struct One<TSubclass,
             typename std::enable_if<
                 std::is_base_of<Element, TSubclass>::value>::type> {
    //! Returns a new instance of the one of \p x.
    TSubclass operator()(TSubclass const& x) const {
      return static_cast<TSubclass>(x.identity());
    }

    //! Returns a new instance of the one of \p x.
    TSubclass operator()(size_t n) {
      return TSubclass(std::move(TSubclass::identity(n)));
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // Product specializations
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter Product for pointers to subclasses of
  //! Element.
  //!
  //! \sa Product.
  template <typename TSubclass>
  struct Product<TSubclass*,
                 typename std::enable_if<
                     std::is_base_of<Element, TSubclass>::value>::type> {
    //! Changes \p xy in-place to hold the product of \p x and \p y using
    //! Element::redefine.
    void operator()(TSubclass*       xy,
                    TSubclass const* x,
                    TSubclass const* y,
                    size_t           tid = 0) {
      xy->redefine(*x, *y, tid);
    }
  };

  //! Specialization of the adapter Product for subclasses of Element.
  //!
  //! \sa Product.
  template <typename TSubclass>
  struct Product<TSubclass,
                 typename std::enable_if<
                     std::is_base_of<Element, TSubclass>::value>::type> {
    //! Changes \p xy in-place to hold the product of \p x and \p y using
    //! Element::redefine.
    void operator()(TSubclass&       xy,
                    TSubclass const& x,
                    TSubclass const& y,
                    size_t           tid = 0) {
      xy.redefine(x, y, tid);
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // Swap specializations
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter Swap for subclasses of Element.
  //!
  //! \sa Swap.
  template <typename TSubclass>
  struct Swap<TSubclass*,
              typename std::enable_if<
                  std::is_base_of<Element, TSubclass>::value>::type> {
    //! Swaps the object pointed to by \p x with the one pointed to by \p y.
    void operator()(TSubclass* x, TSubclass* y) const {
      x->swap(*y);
    }
  };

  // TODO(later) why is there no specialisation for non-pointers for
  // Swap?

  ////////////////////////////////////////////////////////////////////////
  // Inverse specializations
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter Inverse for pointers to
  //! Permutation instances.
  //!
  //! \sa Inverse.
  template <typename TValueType>
  struct Inverse<Permutation<TValueType>*> {
    //! Returns a pointer to a new instance of the inverse of \p x.
    Permutation<TValueType>* operator()(Permutation<TValueType>* x) {
      return new Permutation<TValueType>(std::move(x->inverse()));
    }
  };

  //! Specialization of the adapter Inverse for Permutation instances.
  //!
  //! \sa Inverse.
  template <typename TValueType>
  struct Inverse<Permutation<TValueType>> {
    //! Returns the inverse of \p x.
    Permutation<TValueType> operator()(Permutation<TValueType> const& x) {
      return x.inverse();
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // Hash specializations
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter Hash for subclasses of Element.
  //!
  //! \sa Hash.
  template <typename TSubclass>
  struct Hash<TSubclass,
              typename std::enable_if<
                  std::is_base_of<Element, TSubclass>::value>::type> {
    //! Returns \p x.hash_value()
    size_t operator()(TSubclass const& x) const {
      return x.hash_value();
    }
  };

  //! Specialization of the adapter Hash for pointers to subclasses of Element.
  //!
  //! \sa Hash.
  template <typename TSubclass>
  struct Hash<TSubclass*,
              typename std::enable_if<
                  std::is_base_of<Element, TSubclass>::value>::type> {
    //! Returns \p x->hash_value()
    size_t operator()(TSubclass const* x) const {
      return x->hash_value();
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // EqualTo specializations
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter EqualTo for pointers to subclasses of
  //! Element.
  //!
  //! \sa EqualTo.
  template <typename TSubclass>
  struct EqualTo<
      TSubclass*,
      typename std::enable_if<
          std::is_base_of<libsemigroups::Element, TSubclass>::value>::type> {
    //! Tests equality of two const Element pointers via equality of the
    //! Elements they point to.
    bool operator()(TSubclass const* x, TSubclass const* y) const {
      return *x == *y;
    }
  };

  // TODO(later) why is there no specialisation for non-pointers for
  // EqualTo?

  ////////////////////////////////////////////////////////////////////////
  // ImageRight/LeftAction - PartialPerm
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter ImageRightAction for instance of
  //! PartialPerm.
  // Slowest
  template <typename T>
  struct ImageRightAction<PartialPerm<T>, PartialPerm<T>> {
    //! Stores the idempotent \f$(xy) ^ {-1}xy\f$ in \p res.
    void operator()(PartialPerm<T>&       res,
                    PartialPerm<T> const& pt,
                    PartialPerm<T> const& x) const noexcept {
      res.redefine(pt, x);
      res.swap(res.right_one());
    }
  };

  // Faster than the above, but slower than the below
  // works for T = std::vector and T = StaticVector1
  template <typename S, typename T>
  struct ImageRightAction<PartialPerm<S>, T> {
    void operator()(T& res, T const& pt, PartialPerm<S> const& x) const {
      res.clear();
      for (auto i : pt) {
        if (x[i] != UNDEFINED) {
          res.push_back(x[i]);
        }
      }
      std::sort(res.begin(), res.end());
    }
  };

  // Fastest, but limited to at most degree 64
  template <typename T, size_t N>
  struct ImageRightAction<PartialPerm<T>, BitSet<N>> {
    void operator()(BitSet<N>&            res,
                    BitSet<N> const&      pt,
                    PartialPerm<T> const& x) const {
      res.reset();
      // Apply the lambda to every set bit in pt
      pt.apply([&x, &res](size_t i) {
        if (x[i] != UNDEFINED) {
          res.set(x[i]);
        }
      });
    }
  };

  //! Specialization of the adapter ImageLeftAction for instances of
  //! PartialPerm.
  //!
  //! \sa ImageLeftAction.
  // Slowest
  template <typename T>
  struct ImageLeftAction<PartialPerm<T>, PartialPerm<T>> {
    //! Stores the idempotent \f$xy(xy) ^ {-1}\f$ in \p res.
    void operator()(PartialPerm<T>&       res,
                    PartialPerm<T> const& pt,
                    PartialPerm<T> const& x) const noexcept {
      res.redefine(x, pt);
      res.swap(res.left_one());
    }
  };

  // Fastest when used with BitSet<N>.
  // works for T = std::vector and T = BitSet<N>
  // Using BitSet<N> limits this to size 64. However, if we are trying to
  // compute a LeftAction object, then the max size of such is 2 ^ 64, which is
  // probably not achievable. So, for higher degrees, we will only be able to
  // compute relatively sparse LeftActions (i.e. not containing the majority of
  // the 2 ^ n possible subsets), in which case using vectors or
  // StaticVector1's might be not be appreciable slower anyway. All of this is
  // to say that it probably isn't worthwhile trying to make BitSet's work for
  // more than 64 bits.
  template <typename S, typename T>
  struct ImageLeftAction<PartialPerm<S>, T> {
    void operator()(T& res, T const& pt, PartialPerm<S> const& x) const {
      static PartialPerm<S> xx({});
      x.inverse(xx);
      ImageRightAction<PartialPerm<S>, T>()(res, pt, xx);
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // Lambda/Rho - PartialPerm
  ////////////////////////////////////////////////////////////////////////

  // This currently limits the use of Konieczny to partial perms of degree at
  // most 64 with the default traits class, since we cannot know the degree
  // at compile time, only at run time.
  template <typename T>
  struct LambdaValue<PartialPerm<T>> {
    static constexpr size_t N = BitSet<1>::max_size();
    using type                = BitSet<N>;
  };

  template <typename T>
  struct RhoValue<PartialPerm<T>> {
    using type = typename LambdaValue<PartialPerm<T>>::type;
  };

  template <typename T, size_t N>
  struct Lambda<PartialPerm<T>, BitSet<N>> {
    void operator()(BitSet<N>& res, PartialPerm<T> const& x) const {
      if (x.degree() > N) {
        LIBSEMIGROUPS_EXCEPTION(
            "expected partial perm of degree at most %llu, found %llu",
            N,
            x.degree());
      }
      res.reset();
      for (size_t i = 0; i < x.degree(); ++i) {
        if (x[i] != UNDEFINED) {
          res.set(x[i]);
        }
      }
    }
  };

  template <typename T, size_t N>
  struct Rho<PartialPerm<T>, BitSet<N>> {
    void operator()(BitSet<N>& res, PartialPerm<T> const& x) const {
      if (x.degree() > N) {
        LIBSEMIGROUPS_EXCEPTION(
            "expected partial perm of degree at most %llu, found %llu",
            N,
            x.degree());
      }
      static PartialPerm<T> xx({});
      x.inverse(xx);
      Lambda<PartialPerm<T>, BitSet<N>>()(res, xx);
    }
  };

  template <typename T>
  struct Rank<PartialPerm<T>> {
    size_t operator()(PartialPerm<T> const& x) const {
      return x.crank();
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // ImageRight/LeftAction - Transformation
  ////////////////////////////////////////////////////////////////////////

  // Equivalent to OnSets in GAP
  // Slowest
  // works for T = std::vector and T = StaticVector1
  template <typename S, typename T>
  struct ImageRightAction<Transformation<S>, T> {
    void operator()(T& res, T const& pt, Transformation<S> const& x) const {
      res.clear();
      for (auto i : pt) {
        res.push_back(x[i]);
      }
      std::sort(res.begin(), res.end());
      res.erase(std::unique(res.begin(), res.end()), res.end());
    }

    T operator()(T const& pt, Transformation<S> const& x) const {
      T     res;
      this->operator()(res, pt, x);
      return res;
    }
  };

  // Fastest, but limited to at most degree 64
  template <typename TIntType, size_t N>
  struct ImageRightAction<Transformation<TIntType>, BitSet<N>> {
    void operator()(BitSet<N>&                      res,
                    BitSet<N> const&                pt,
                    Transformation<TIntType> const& x) const {
      res.reset();
      // Apply the lambda to every set bit in pt
      pt.apply([&x, &res](size_t i) { res.set(x[i]); });
    }
  };

  // OnKernelAntiAction
  // T = StaticVector1<S> or std::vector<S>
  template <typename S, typename T>
  struct ImageLeftAction<Transformation<S>, T> {
    void operator()(T& res, T const& pt, Transformation<S> const& x) const {
      res.clear();
      res.resize(x.degree());
      static thread_local std::vector<S> buf;
      buf.clear();
      buf.resize(x.degree(), S(UNDEFINED));
      S next = 0;

      for (size_t i = 0; i < res.size(); ++i) {
        if (buf[pt[x[i]]] == UNDEFINED) {
          buf[pt[x[i]]] = next++;
        }
        res[i] = buf[pt[x[i]]];
      }
    }

    T operator()(T const& pt, Transformation<S> const& x) const {
      T     res;
      this->operator()(res, pt, x);
      return res;
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // Lambda/Rho - Transformation
  ////////////////////////////////////////////////////////////////////////

  // This currently limits the use of Konieczny to transformation of degree at
  // most 64 with the default traits class, since we cannot know the degree at
  // compile time, only at run time.
  template <typename T>
  struct LambdaValue<Transformation<T>> {
    static constexpr size_t N = BitSet<1>::max_size();
    using type                = BitSet<N>;
  };

  // Benchmarks indicate that using std::vector yields similar performance to
  // using StaticVector1's.
  template <typename T>
  struct RhoValue<Transformation<T>> {
    using type = std::vector<T>;
  };

  // T = std::vector or StaticVector1
  template <typename S, typename T>
  struct Lambda<Transformation<S>, T> {
    // not noexcept because std::vector::resize isn't (although
    // StaticVector1::resize is).
    void operator()(T& res, Transformation<S> const& x) const {
      res.clear();
      res.resize(x.degree());
      for (size_t i = 0; i < res.size(); ++i) {
        res[i] = x[i];
      }
      std::sort(res.begin(), res.end());
      res.erase(std::unique(res.begin(), res.end()), res.end());
    }
  };

  template <typename T, size_t N>
  struct Lambda<Transformation<T>, BitSet<N>> {
    // not noexcept because it can throw
    void operator()(BitSet<N>& res, Transformation<T> const& x) const {
      if (x.degree() > N) {
        LIBSEMIGROUPS_EXCEPTION(
            "expected a transformation of degree at most %llu, found %llu",
            N,
            x.degree());
      }
      res.reset();
      for (size_t i = 0; i < x.degree(); ++i) {
        res.set(x[i]);
      }
    }
  };

  // T = std::vector<S> or StaticVector1<S, N>
  template <typename S, typename T>
  struct Rho<Transformation<S>, T> {
    // not noexcept because std::vector::resize isn't (although
    // StaticVector1::resize is).
    void operator()(T& res, Transformation<S> const& x) const {
      res.clear();
      res.resize(x.degree());
      static thread_local std::vector<S> buf;
      buf.clear();
      buf.resize(x.degree(), S(UNDEFINED));
      S next = 0;

      for (size_t i = 0; i < res.size(); ++i) {
        if (buf[x[i]] == UNDEFINED) {
          buf[x[i]] = next++;
        }
        res[i] = buf[x[i]];
      }
    }
  };

  template <typename T>
  struct Rank<Transformation<T>> {
    size_t operator()(Transformation<T> const& x) {
      return x.crank();
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // ImageRightAction - Permutation specialisation
  ////////////////////////////////////////////////////////////////////////

  //! Specialization of the adapter ImageRightAction for pointers to
  //! Permutation instances.
  //!
  //! \sa ImageRightAction.
  template <typename TValueType>
  struct ImageRightAction<
      Permutation<TValueType>*,
      TValueType,
      typename std::enable_if<std::is_integral<TValueType>::value>::type> {
    //! Returns the image of \p pt under \p x.
    TValueType operator()(Permutation<TValueType> const* x,
                          TValueType const               pt) {
      return (*x)[pt];
    }
  };

  //! Specialization of the adapter ImageRightAction for instances of
  //! Permutation.
  //!
  //! \sa ImageRightAction.
  template <typename TIntType>
  struct ImageRightAction<
      Permutation<TIntType>,
      TIntType,
      typename std::enable_if<std::is_integral<TIntType>::value>::type> {
    //! Stores the image of \p pt under the action of \p p in \p res.
    void operator()(TIntType&                    res,
                    TIntType const&              pt,
                    Permutation<TIntType> const& p) const noexcept {
      LIBSEMIGROUPS_ASSERT(pt < p.degree());
      res = p[pt];
    }

    //! Returns the image of \p pt under the action of \p p.
    TIntType operator()(TIntType const pt, Permutation<TIntType> const& x) {
      return x[pt];
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // ImageRight/LeftAction - BooleanMat
  ////////////////////////////////////////////////////////////////////////

  // T = StaticVector1<BitSet<N>, N> or std::vector<BitSet<N>>
  // Possibly further container when value_type is BitSet.
  template <typename T>
  struct ImageRightAction<BooleanMat, T> {
    // not noexcept because BitSet<N>::apply isn't
    void operator()(T& res, T const& pt, BooleanMat const& x) const {
      using value_type = typename T::value_type;
      // TODO(later):
      // static_assert(std::is_same<BitSet, value_type>::value(),
      //              "the template paramater T must be a container of
      //              BitSet's");
      // The above doesn't work because BitSet requires template parameters
      res.clear();

      for (auto const& v : pt) {
        value_type cup;
        cup.reset();
        v.apply([&x, &cup](size_t i) {
          for (size_t j = 0; j < x.degree(); ++j) {
            cup.set(j, cup[j] || x[i * x.degree() + j]);
          }
        });
        res.push_back(std::move(cup));
      }
      booleanmat_helpers::rows_basis<T>(res);
    }
  };

  template <>
  struct ImageRightAction<BooleanMat, std::vector<std::vector<bool>>> {
    // not noexcept because the constructor of std::vector isn't
    void operator()(std::vector<std::vector<bool>>&       res,
                    std::vector<std::vector<bool>> const& pt,
                    BooleanMat const&                     x) const {
      res.clear();

      for (auto it = pt.cbegin(); it < pt.cend(); ++it) {
        std::vector<bool> cup(x.degree(), false);
        for (size_t i = 0; i < x.degree(); ++i) {
          if ((*it)[i]) {
            for (size_t j = 0; j < x.degree(); ++j) {
              cup[j] = cup[j] || x[i * x.degree() + j];
            }
          }
        }
        res.push_back(std::move(cup));
      }
      booleanmat_helpers::rows_basis(res);
    }
  };

  template <typename T>
  struct ImageLeftAction<BooleanMat, T> {
    void operator()(T& res, T const& pt, BooleanMat const& x) const {
      const_cast<BooleanMat*>(&x)->transpose_in_place();
      // What if ImageRightAction throws?
      ImageRightAction<BooleanMat, T>()(res, pt, x);
      const_cast<BooleanMat*>(&x)->transpose_in_place();
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // Lambda/Rho - BooleanMat
  ////////////////////////////////////////////////////////////////////////

  // This currently limits the use of Konieczny to matrices of dimension at
  // most 64 with the default traits class, since we cannot know the dimension
  // of the matrices at compile time, only at run time.
  template <>
  struct LambdaValue<BooleanMat> {
    static constexpr size_t N = BitSet<1>::max_size();
    using type                = detail::StaticVector1<BitSet<N>, N>;
  };

  template <>
  struct RhoValue<BooleanMat> {
    using type = typename LambdaValue<BooleanMat>::type;
  };

  // Benchmarks show that the following is the fastest (i.e. duplicating the
  // code from ImageRightAction, and using StaticVector1).
  // T = StaticVector1<BitSet>, std::vector<BitSet>,
  // StaticVector1<std::bitset>, std::vector<std::bitset>
  template <typename T>
  struct Lambda<BooleanMat, T> {
    void operator()(T& res, BooleanMat const& x) const {
      using S        = typename T::value_type;
      size_t const N = S().size();
      if (x.degree() > N) {
        LIBSEMIGROUPS_EXCEPTION(
            "expected matrix of dimension at most %llu, found %llu",
            N,
            x.degree());
      }
      res.clear();
      for (size_t i = 0; i < x.degree(); ++i) {
        S cup;
        cup.reset();
        for (size_t j = 0; j < x.degree(); ++j) {
          cup.set(j, x[i * x.degree() + j]);
        }
        res.push_back(std::move(cup));
      }
      booleanmat_helpers::rows_basis<T>(res);
    }
  };

  // T = StaticVector1<BitSet>, std::vector<BitSet>,
  // StaticVector1<std::bitset>, std::vector<std::bitset>
  template <typename T>
  struct Rho<BooleanMat, T> {
    void operator()(T& res, BooleanMat const& x) const noexcept {
      const_cast<BooleanMat*>(&x)->transpose_in_place();
      Lambda<BooleanMat, T>()(res, x);
      const_cast<BooleanMat*>(&x)->transpose_in_place();
    }
  };

  // Version of ImageRightAction where we convert the matrix to bitsets
  template <size_t N>
  struct ImageRightAction<BooleanMat, BitSet<N>> {
    using result_type = BitSet<N>;
    void operator()(result_type&       res,
                    result_type const& pt,
                    BooleanMat const&  x) const {
      static thread_local detail::StaticVector1<BitSet<N>, N> x_rows;
      x_rows.clear();
      for (size_t i = 0; i < x.degree(); ++i) {
        BitSet<N> row(0);
        for (size_t j = 0; j < x.degree(); ++j) {
          if (x[i * x.degree() + j]) {
            row.set(j, true);
          }
        }
        x_rows.push_back(std::move(row));
      }
      res.reset();
      pt.apply([&res](size_t i) { res |= x_rows[i]; });
    }
  };

  template <>
  class RankState<BooleanMat> {
    using MaxBitSet = BitSet<BitSet<1>::max_size()>;

   public:
    using type = RightAction<BooleanMat,
                             MaxBitSet,
                             ImageRightAction<BooleanMat, MaxBitSet>>;
    template <typename T>
    RankState(T first, T last) : _orb() {
      static thread_local std::vector<MaxBitSet> seeds;
      LIBSEMIGROUPS_ASSERT(_orb.empty());
      LIBSEMIGROUPS_ASSERT(_orb.started());
      if (std::distance(first, last) == 0) {
        LIBSEMIGROUPS_EXCEPTION(
            "expected a positive number of generators in the second argument");
      }
      for (auto it = first; it < last; ++it) {
        _orb.add_generator(*it);
      }
      for (size_t i = 0; i < first->degree(); ++i) {
        MaxBitSet seed(0);
        seed.set(i, true);
        _orb.add_seed(seed);
      }
    }

    type const& get() const {
      _orb.run();
      LIBSEMIGROUPS_ASSERT(_orb.finished());
      return _orb;
    }

   private:
    mutable type _orb;
  };

  template <>
  struct Rank<BooleanMat> {
    size_t operator()(RankState<BooleanMat> const& state,
                      BooleanMat const&            x) const {
      using bs       = BitSet<BitSet<1>::max_size()>;
      using orb_type = typename RankState<BooleanMat>::type;
      static thread_local std::vector<size_t> vec;
      static thread_local std::vector<bs>     x_rows;
      vec.clear();
      x_rows.clear();
      orb_type const& orb = state.get();
      LIBSEMIGROUPS_ASSERT(orb.finished());
      vec.resize(orb.current_size());
      for (size_t i = 0; i < x.degree(); ++i) {
        bs row(0);
        for (size_t j = 0; j < x.degree(); ++j) {
          if (x[i * x.degree() + j]) {
            row.set(j, true);
          }
        }
        x_rows.push_back(std::move(row));
      }
      for (size_t i = 0; i < orb.current_size(); ++i) {
        bs const& row = orb[i];
        bs        cup;
        cup.reset();
        row.apply([&cup](size_t i) { cup |= x_rows[i]; });
        LIBSEMIGROUPS_ASSERT(orb.position(cup) != UNDEFINED);
        vec.push_back(orb.position(cup));
      }
      return Transformation<size_t>(vec).crank();
    }
  };
}  // namespace libsemigroups
#endif  // LIBSEMIGROUPS_ELEMENT_ADAPTERS_HPP_
