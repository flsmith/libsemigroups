//
// libsemigroups - C++ library for semigroups and monoids
// Copyright (C) 2016-18 James D. Mitchell
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

// This file contains the declaration of the element class and its subclasses.

#ifndef LIBSEMIGROUPS_SRC_ELEMENTS_H_
#define LIBSEMIGROUPS_SRC_ELEMENTS_H_

#include <math.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <unordered_set>
#include <vector>

#include "blocks.h"
#include "libsemigroups-debug.h"
#include "libsemigroups-exception.h"
#include "recvec.h"
#include "semiring.h"

#ifdef LIBSEMIGROUPS_USE_HPCOMBI
#include "../extern/HPCombi/include/perm16.hpp"
#endif

namespace libsemigroups {

  //! Abstract base class for semigroup elements
  //!
  //! The Semigroup class consists of Element objects. Every derived class of
  //! Element implements the deleted methods of Element, which are used by the
  //! Semigroup class.
  class Element {
   public:
    //! A constructor.
    Element() : _hash_value(UNDEFINED) {}
    explicit Element(size_t hv) : _hash_value(hv) {}

    //! A default destructor.
    virtual ~Element() {}

    //! Returns \c true if \c this equals \p that.
    //!
    //! This method checks the mathematical equality of two Element objects in
    //! the same subclass of Element.
    virtual bool operator==(Element const& that) const = 0;

    bool operator!=(Element const& that) const {
      return !(*this == that);
    }

    //! Returns \c true if \c this is less than \p that.
    //!
    //! This method defines a total order on the set of objects in a given
    //! subclass of Element with a given Element::degree. The definition of
    //! this total order depends on the method for the operator < in the
    //! subclass.
    virtual bool operator<(const Element& that) const = 0;

    bool operator>(const Element& that) const {
      return that < *this;
    }

    //! Returns the approximate time complexity of multiplying two
    //! Element objects in a given subclass.
    //!
    //! This method returns an integer which represents the approximate time
    //! complexity of multiplying two objects in the same subclass of Element
    //! which have the same Element::degree. For example, the approximate time
    //! complexity of multiplying two \f$3\times 3\f$ matrices over a common
    //! semiring is \f$O(3 ^ 3)\f$, and 27 is returned by
    //! MatrixOverSemiring::complexity.
    //!
    //! The returned value is used in, for example, Semigroup::fast_product and
    //! Semigroup::nridempotents to decide if it is better to multiply
    //! elements or follow a path in the Cayley graph.
    virtual size_t complexity() const = 0;

    //! Returns the degree of an Element.
    //!
    //! This method returns an integer which represents the size of the
    //! element, and is used to determine whether or not two elements are
    //! compatible for multiplication. For example, two Transformation objects
    //! of different degrees cannot be multiplied, and a Bipartition of degree
    //! 10 cannot be an element of a monoid of bipartitions of degree 3.
    //!
    //! See the relevant subclass for the particular meaning of the return value
    //! of this method for each subclass.
    virtual size_t degree() const = 0;

    //! Return the hash value of an Element.
    //!
    //! This method returns a hash value for an object in a subclass of
    //! Element. This value is only computed the first time this method is
    //! called.
    inline size_t hash_value() const {
      if (_hash_value == UNDEFINED) {
        this->cache_hash_value();
      }
      return this->_hash_value;
    }

    //! Returns a new copy of the identity element.
    //!
    //! This method returns the multiplicative identity element for an object
    //! in a subclass of Element. The returned identity belongs to the same
    //! subclass and has the same degree as \c this.
    // FIXME remove this? I can't see how at present.
    // virtual Element* identity() const = 0;

    //! Copy another Element into \c this.
    //!
    //! This method copies its argument into \c this by changing \c this
    //! in-place.
    virtual void copy(Element const&) = 0;

    void copy(Element const* x) {
      return copy(*x);
    }

    //! Swap another Element with \c this.
    //!
    //! This method swaps the defining data of \p x and \c this.
    virtual void swap(Element& x) = 0;

    //! Multiplies \p x and \p y and stores the result in \c this.
    //!
    //! Redefine \c this to be the product of \p x and \p y. This is in-place
    //! multiplication to avoid allocation of memory for products which do not
    //! need to be stored for future use.
    //!
    //! The implementation of this method in the Element base class simply
    //! calls the 3 parameter version with third parameter 0. Any subclass of
    //! Element can implement either a two or three parameter version of this
    //! method and the base class method implements the other method.
    virtual void redefine(Element const& x, Element const& y) {
      redefine(x, y, 0);
    }

    void redefine(Element const* x, Element const* y) {
      redefine(*x, *y, 0);
    }

    //! Multiplies \p x and \p y and stores the result in \c this.
    //!
    //! Redefine \c this to be the product of \p x and \p y. This is in-place
    //! multiplication to avoid allocation of memory for products which do not
    //! need to be stored for future use.
    //!
    //! The implementation of this method in the Element base class simply
    //! calls the 2 parameter version and ignores the third parameter \p
    //! thread_id. Any subclass of Element can implement either a two or three
    //! parameter version of this method and the base class method implements
    //! the other method.
    //!
    //! The parameter \p thread_id is required in some derived classes of
    //! Element because some temporary storage is required to find the product
    //! of \p x and \p y.
    //!
    //! Note that if different threads call this method on a derived class of
    //! Element where static temporary storage is used in the redefine method
    //! with the same value of \p thread_id, then bad things may happen.
    virtual void redefine(Element const& x, Element const& y, size_t) {
      redefine(x, y);
    }

    void redefine(Element const* x, Element const* y, size_t) {
      redefine(*x, *y);
    }

#if defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    && defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
    virtual Element* empty_key() const = 0;
#endif

    virtual void increase_deg_by(size_t) {}

    //! Returns a new element completely independent of \c this.
    //!
    //! This method really copies an Element. To minimise the amount of copying
    //! when Element objects are inserted in a std::unordered_map and other
    //! containers, an Element behaves somewhat like a pointer, in that the
    //! actual data in an Element is only copied when this method is called.
    //! Otherwise, if an Element is copied, then its defining data is only
    //! stored once.
    virtual Element* heap_copy() const = 0;

    virtual Element* heap_identity() const = 0;

   protected:
    //! Calculate and cache a hash value.
    //!
    //! This method is used to compute and cache the hash value of \c this.
    virtual void cache_hash_value() const = 0;

    //! Reset the cached value used by Element::hash_value.
    //!
    //! This method is used to reset the cached hash value to
    //! libsemigroups::Element::UNDEFINED. This is required after running
    //! Element::redefine, Element::copy, or any other method that changes the
    //! defining data of \c this.
    void reset_hash_value() const {
      _hash_value = UNDEFINED;
    }

    //! UNDEFINED value.
    //!
    //! This variable is used to indicate that a value is undefined. For
    //! example, the cached hash value of an Element is initially set to this
    //! value.
    // FIXME move to constants.h file
    static size_t const UNDEFINED;

    //! This data member holds a cached version of the hash value of an Element.
    //! It is stored here if it is ever computed. It is invalidated by
    //! libsemigroups::Element::redefine and sometimes by
    //! libsemigroups::Element::really_copy, and potentially any other
    //! non-const data member of Element or any of its subclasses.
    mutable size_t _hash_value;
  };

  //! Abstract base class for elements using a vector to store their defining
  //! data.
  //!
  //! The template parameter \p TValueType is the type entries in the vector
  //! containing the defining data.
  //!
  //! The template parameter \p TSubclass is the subclass of
  //! ElementWithVectorData used by certain methods to construct new instances
  //! of subclasses of ElementWithVectorData.
  //!
  //! For example, Transformation&lt;u_int128_t&gt; is a subclass of
  //! ElementWithVectorData&lt;u_int128_t, Transformation&lt;u_int128_t&gt;&gt;
  //! so that when the identity method in this class is called it returns a
  //! Transformation and not an ElementWithVectorData.
  template <typename TValueType, class TSubclass>
  class ElementWithVectorData : public Element {
   public:
    //! A constructor.
    //!
    //! Returns an object with an uninitialised vector.
    ElementWithVectorData() : Element(), _vector() {}

    explicit ElementWithVectorData(size_t n) : Element(), _vector(n) {}

    //! A constructor.
    //!
    //! The parameter \p vector should be a const reference to defining data of
    //! the element.
    //!
    //! Returns an object whose defining data is a copy of \p vector.
    explicit ElementWithVectorData(std::vector<TValueType> const& vector)
        : Element(), _vector(vector) {}

    //! A copy constructor.
    //!
    //! The size of the vector containing the defining data of \c this will be
    //! increased by \p increase_deg_by.  If \p increase_deg_by is not 0, then
    //! this method must be overridden by any subclass of ElementWithVectorData
    //! since there is no way of knowing how a subclass is defined by the data
    //! in the vector.
    ElementWithVectorData(ElementWithVectorData const& copy)
        : Element(copy._hash_value),
          _vector(copy._vector.cbegin(), copy._vector.cend()) {}

    TSubclass operator*(ElementWithVectorData const& y) const {
      TSubclass xy(y.degree());
      xy.redefine(*this, y);
      return xy;
    }

    //! Returns the \p pos entry in the vector containing the defining data.
    //!
    //! This method returns the \p pos entry in the vector used to construct \c
    //! this. No checks are performed that \p pos in within the bounds of this
    //! vector.
    inline TValueType operator[](size_t pos) const {
      return _vector[pos];
    }

    //! Returns the \p pos entry in the vector containing the defining data.
    //!
    //! This method returns the \p pos entry in the vector used to construct \c
    //! this.
    inline TValueType at(size_t pos) const {
      return _vector.at(pos);
    }

    //! Returns \c true if \c this equals \p that.
    //!
    //! This method checks that the underlying vectors of \c this and \p that
    //! are equal.
    bool operator==(Element const& that) const override {
      return static_cast<ElementWithVectorData const&>(that)._vector
             == this->_vector;
    }

    //! Returns \c true if \c this is less than \p that.
    //!
    //! This method defines a total order on the set of objects in
    //! ElementWithVectorData of a given Element::degree, which is the
    //! short-lex order.
    bool operator<(Element const& that) const override {
      auto& ewvd = static_cast<ElementWithVectorData const&>(that);
      if (this->_vector.size() != ewvd._vector.size()) {
        return this->_vector.size() < ewvd._vector.size();
      }
      for (size_t i = 0; i < this->_vector.size(); i++) {
        if ((*this)[i] != ewvd[i]) {
          return (*this)[i] < ewvd[i];
        }
      }
      return false;
    }

    //! Copy another Element into \c this.
    //!
    //! This method copies \p x into \c this by changing \c this in-place. This
    //! method asserts that the degrees of \c this and \p x are equal and then
    //! replaces the underlying vector of \c this with the underlying vector of
    //! \p x. Any method overriding this one must define _hash_value correctly
    //! or call Element::reset_hash_value on \c this.
    void copy(Element const& x) override {
      LIBSEMIGROUPS_ASSERT(x.degree() == this->degree());
      auto const& xx = static_cast<ElementWithVectorData const&>(x);
      _vector.assign(xx._vector.cbegin(), xx._vector.cend());
      this->_hash_value = xx._hash_value;
    }

    //! Swap another Element with \c this.
    //!
    //! This method swaps the defining data of \p x and \c this.
    //! This method asserts that the degrees of \c this and \p x are equal and
    //! then swaps the underlying vector of \c this with the underlying vector
    //! of \p x. Any method overriding should swap the hash_value of \c this
    //! and \p x, or call Element::reset_hash_value on \c this and \p x.
    void swap(Element& x) override {
      LIBSEMIGROUPS_ASSERT(x.degree() == this->degree());
      auto& xx = static_cast<ElementWithVectorData&>(x);
      _vector.swap(xx._vector);
      std::swap(this->_hash_value, xx._hash_value);
    }

#if defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    && defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
    TSubclass empty_key() const override {
      return TSubclass({std::numeric_limits<TValueType>::max()});
    }
#endif

    //! Returns an iterator.
    //!
    //! This method returns an iterator pointing at the first entry in the
    //! vector that is the underlying defining data of \c this.
    inline typename std::vector<TValueType>::const_iterator begin() const {
      return _vector.begin();
    }

    //! Returns an iterator.
    //!
    //! This method returns an iterator referring to the past-the-end element
    //! of the vector that is the underlying defining data of \c this.
    inline typename std::vector<TValueType>::const_iterator end() const {
      return _vector.end();
    }

    //! Returns a const iterator.
    //!
    //! This method returns a const_iterator pointing at the first entry in the
    //! vector that is the underlying defining data of \c this.
    inline typename std::vector<TValueType>::iterator cbegin() const {
      return _vector.cbegin();
    }

    //! Returns a const iterator.
    //!
    //! This method returns a const iterator referring to the past-the-end
    //! element of the vector that is the underlying defining data of \c this.
    inline typename std::vector<TValueType>::iterator cend() const {
      return _vector.cend();
    }

    // FIXME why does this method exist?
    inline size_t size() const {
      return _vector.size();
    }

    virtual TSubclass identity() const = 0;

    Element* heap_identity() const override {
      return this->identity().heap_copy();
    }

    Element* heap_copy() const override {
      return new TSubclass(*static_cast<TSubclass const*>(this));
    }

   protected:
    // Cannot declare cache_hash_value here, since PBR's are vectors of
    // vectors, and there is not std::hash<vector<whatever>>.
    //! Returns a hash value for a vector provided there is a specialization of
    //! std::hash for the template type \p T.
    template <typename T>
    static inline size_t vector_hash(std::vector<T> const& vec) {
      size_t seed = 0;
      for (auto const& x : vec) {
        seed ^= std::hash<T>{}(x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }

    //! The vector containing the defining data of \c this.
    //!
    //! The actual data defining of \c this is stored in _vector.
    std::vector<TValueType> _vector;
  };

  //! Abstract base class for elements using a vector to store their defining
  //! data and the default hash function for that underlying vector.
  //!
  //! This class is almost the same as ElementWithVectorData, except that it
  //! also implements a method for cache_hash_value, which uses
  //! ElementWithVectorData::vector_hash.
  template <typename TValueType, class TSubclass>
  class ElementWithVectorDataDefaultHash
      : public ElementWithVectorData<TValueType, TSubclass> {
   public:
    using ElementWithVectorData<TValueType, TSubclass>::ElementWithVectorData;
    ElementWithVectorDataDefaultHash()
        : ElementWithVectorData<TValueType, TSubclass>() {}

   protected:
    //! This method implements the default hash function for an
    //! ElementWithVectorData, which uses ElementWithVectorData::vector_hash.
    //! Derive from this class if you are defining a class derived from
    //! ElementWithVectorData and there is a specialization of std::hash for
    //! the template parameter \p TValueType, and you do not want to define a
    //! hash function explicitly in your derived class.
    void cache_hash_value() const override {
      this->_hash_value = this->vector_hash(this->_vector);
    }
  };

  //! Abstract class for partial transformations.
  //!
  //! This is a template class for partial transformations, which is a subclass
  //! of ElementWithVectorDataDefaultHash. For example,
  //! Transformation<u_int128_t> is a subclass of
  //! PartialTransformation<u_int128_t, Transformation<u_int128_t>>.
  //!
  //! The template parameter \p TValueType is the type of image values, i.e.
  //! u_int16_t, and so on.  The value of the template parameter \p S can be
  //! used to reduce (or increase) the amount of memory required by instances
  //! of this class.
  //!
  //! The template parameter \p TSubclass is the subclass of
  //! PartialTransformation used by the PartialTransformation::identity method
  //! to construct an identity, so that the return value of the method
  //! PartialTransformation::identity is of type \p TSubclass instead of type
  //! PartialTransformation.
  //!
  //! This class is abstract since it does not implement all methods required
  //! by the Element class, it exists to provide common methods for its
  //! subclasses.
  //!
  //! A *partial transformation* \f$f\f$ is just a function defined on a subset
  //! of \f$\{0, 1, \ldots, n - 1\}\f$ for some integer \f$n\f$ called the
  //! *degree*  of *f*.  A partial transformation is stored as a vector of the
  //! images of \f$\{0, 1, \ldots, n -1\}\f$, i.e.
  //! \f$\{(0)f, (1)f, \ldots, (n - 1)f\}\f$
  //! where the value PartialTransformation::UNDEFINED is used to
  //! indicate that \f$(i)f\f$ is, you guessed it, undefined (i.e. not among
  //! the points where \f$f\f$ is defined).
  template <typename TValueType, typename TSubclass>
  class PartialTransformation
      : public ElementWithVectorDataDefaultHash<TValueType, TSubclass> {
   public:
    using ElementWithVectorDataDefaultHash<TValueType, TSubclass>::
        ElementWithVectorDataDefaultHash;

    PartialTransformation()
        : ElementWithVectorDataDefaultHash<TValueType, TSubclass>() {}

    explicit PartialTransformation(std::vector<TValueType> const& vec)
        : ElementWithVectorDataDefaultHash<TValueType, TSubclass>(vec) {
      validate();
    }

    PartialTransformation(std::initializer_list<TValueType> imgs)
        : PartialTransformation<TValueType, TSubclass>(
              std::vector<TValueType>(imgs)) {}

    void validate() const {
      for (auto const& val : this->_vector) {
        if ((val < 0 || val >= this->degree()) && val != UNDEFINED) {
          throw LibsemigroupsException(
              "PartialTransformation: image value out of bounds, found "
              + std::to_string(val) + ", must be less than "
              + std::to_string(this->degree()));
        }
      }
    }

    //! Returns the approximate time complexity of multiplying two
    //! partial transformations.
    //!
    //! The approximate time complexity of multiplying partial transformations
    //! is just their degree.
    size_t complexity() const override {
      return this->_vector.size();
    }

    //! Returns the degree of a partial transformation.
    //!
    //! The *degree* of a partial transformation is the number of points used
    //! in its definition, which is equal to the size of
    //! ElementWithVectorData::_vector.
    size_t degree() const override {
      return this->_vector.size();
    }

    //! Returns the rank of a partial transformation.
    //!
    //! The *rank* of a partial transformation is the number of its distinct
    //! image values, not including PartialTransformation::UNDEFINED. This
    //! method recomputes the return value every time it is called.
    size_t crank() const {
      _lookup.clear();
      _lookup.resize(degree(), false);
      size_t r = 0;
      for (auto const& x : this->_vector) {
        if (x != UNDEFINED && !_lookup[x]) {
          _lookup[x] = true;
          r++;
        }
      }
      return r;
    }

    //! Returns the identity transformation with degrees of \c this.
    //!
    //! This method returns a new partial transformation with degree equal to
    //! the degree of \c this that fixes every value from *0* to the degree of
    //! \c this.
    TSubclass identity() const override {
      std::vector<TValueType> vector;
      vector.reserve(this->degree());
      for (size_t i = 0; i < this->degree(); i++) {
        vector.push_back(i);
      }
      return TSubclass(vector);
    }

    //! Undefined image value.
    //!
    //! This value is used to indicate that a partial transformation is not
    //! defined on a value.
    static TValueType const UNDEFINED;

   private:
    // Used for determining rank
    static std::vector<bool> _lookup;
  };

  template <typename TValueType, typename TSubclass>
  std::vector<bool> PartialTransformation<TValueType, TSubclass>::_lookup
      = std::vector<bool>();

  template <typename TValueType, typename TSubclass>
  TValueType const PartialTransformation<TValueType, TSubclass>::UNDEFINED
      = std::numeric_limits<TValueType>::max();

  //! Template class for transformations.
  //!
  //! The value of the template parameter \p T can be used to reduce the amount
  //! of memory required by instances of this class; see PartialTransformation
  //! and ElementWithVectorData for more details.
  //!
  //! A *transformation* \f$f\f$ is just a function defined on the whole of
  //! \f$\{0, 1, \ldots, n - 1\}\f$ for some integer \f$n\f$ called the
  //! *degree* of \f$f\f$.  A transformation is stored as a vector of the
  //! images of \f$\{0, 1, \ldots, n - 1\}\f$, i.e.
  //! \f$\{(0)f, (1)f, \ldots, (n - 1)f\}\f$.
  template <typename T>
  class Transformation : public PartialTransformation<T, Transformation<T>> {
   public:
    using PartialTransformation<T, Transformation<T>>::PartialTransformation;
    Transformation() : PartialTransformation<T, Transformation<T>>() {}

    explicit Transformation(std::vector<T> const& vec)
        : PartialTransformation<T, Transformation<T>>(vec) {
#if !defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    || !defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
      validate();
#endif
    }

    Transformation(std::initializer_list<T> imgs)
        : Transformation<T>(std::vector<T>(imgs)) {}

    void validate() const {
      size_t max = this->_vector.size();
      for (auto const& val : this->_vector) {
        if (val < 0 || val >= max) {
          throw LibsemigroupsException(
              "Transformation: image value out of bounds, found "
              + std::to_string(val) + ", must be less than "
              + std::to_string(max));
        }
      }
    }

    //! A copy constructor.
    Transformation<T>(Transformation<T> const& copy)
        : PartialTransformation<T, Transformation<T>>(copy) {}

    void increase_deg_by(size_t m) override {
      this->_vector.resize(this->_vector.size() + m);
      std::iota(this->_vector.end() - m,
                this->_vector.end(),
                this->_vector.size() - m);
      this->reset_hash_value();
    }

    //! Multiply \p x and \p y and stores the result in \c this.
    //!
    //! See Element::redefine for more details about this method.
    //!
    //! This method asserts that the degrees of \p x, \p y, and \c this, are
    //! all equal, and that neither \p x nor \p y equals \c this.
    void redefine(Element const& x, Element const& y) override {
      LIBSEMIGROUPS_ASSERT(x.degree() == y.degree());
      LIBSEMIGROUPS_ASSERT(x.degree() == this->degree());
      LIBSEMIGROUPS_ASSERT(&x != this && &y != this);
      auto const&  xx = static_cast<Transformation<T> const&>(x);
      auto const&  yy = static_cast<Transformation<T> const&>(y);
      size_t const n  = this->_vector.size();
      for (T i = 0; i < n; i++) {
        (this->_vector)[i] = yy[xx[i]];
      }
      this->reset_hash_value();
    }

   protected:
    //! This method is included because it seems to give superior performance
    //! in some benchmarks.
    void cache_hash_value() const override {
      size_t seed = 0;
      size_t deg  = this->_vector.size();
      for (auto const& val : this->_vector) {
        seed *= deg;
        seed += val;
      }
      this->_hash_value = seed;
    }
  };

  //! Template class for partial permutations.
  //!
  //! The value of the template parameter \p T can be used to reduce the
  //! amount of memory required by instances of this class; see
  //! PartialTransformation and ElementWithVectorData for more details.
  //!
  //! A *partial permutation* \f$f\f$ is just an injective partial
  //! transformation, which is stored as a vector of the images of
  //! \f$\{0, 1, \ldots, n - 1\}\f$, i.e.
  //! i.e. \f$\{(0)f, (1)f, \ldots, (n - 1)f\}\f$ where the value
  //! PartialTransformation::UNDEFINED is
  //! used to indicate that \f$(i)f\f$ is undefined (i.e. not among
  //! the points where \f$f\f$ is defined).
  template <typename T>
  class PartialPerm : public PartialTransformation<T, PartialPerm<T>> {
   public:
    using PartialTransformation<T, PartialPerm<T>>::PartialTransformation;
    using PartialTransformation<T, PartialPerm<T>>::UNDEFINED;

    explicit PartialPerm(std::vector<T> const& vec)
        : PartialTransformation<T, PartialPerm<T>>(vec) {
      validate();
    }

    PartialPerm(std::initializer_list<T> imgs)
        : PartialPerm<T>(std::vector<T>(imgs)) {}

    //! A constructor.
    //!
    //! Constructs a partial perm of degree \p deg such that \code (dom[i])f =
    //! ran[i] \endcode for all \c i and which is undefined on every other
    //! value in the range 0 to (strictly less than \p deg). This method
    //! asserts that \p dom and \p ran have equal size and that \p deg is
    //! greater than to the maximum value in \p dom or \p ran.
    PartialPerm(std::vector<T> const& dom,
                std::vector<T> const& ran,
                size_t                deg)
        // The vector passed in the next line shouldn't be necessary, but with
        // GCC5 PartialPerm fails to inherit the 0-param constructor from
        // PartialTransformation.
        : PartialTransformation<T, PartialPerm<T>>(std::vector<T>()) {
      if (dom.size() != ran.size()) {
        throw LibsemigroupsException(
            "PartialPerm: domain and range size mismatch");
      } else if (!(dom.empty()
                   || deg > *std::max_element(dom.cbegin(), dom.cend()))) {
        throw LibsemigroupsException(
            "PartialPerm: domain value out of bounds, found "
            + std::to_string(*std::max_element(dom.cbegin(), dom.cend()))
            + ", must be less than " + std::to_string(deg));
      }

      this->_vector.resize(deg, UNDEFINED);
      for (size_t i = 0; i < dom.size(); i++) {
        this->_vector[dom[i]] = ran[i];
      }
      validate();
    }

    PartialPerm(std::initializer_list<T> dom,
                std::initializer_list<T> ran,
                size_t                   deg)
        : PartialPerm<T>(std::vector<T>(dom), std::vector<T>(ran), deg) {}

    void validate() const {
      std::vector<bool> present(this->degree(), false);
      for (auto const& val : this->_vector) {
        if (val != UNDEFINED) {
          if (val < 0 || val >= this->degree()) {
            throw LibsemigroupsException(
                "PartialPerm: image value out of bounds, found "
                + std::to_string(val) + ", must be less than "
                + std::to_string(this->degree()));
          } else if (present[val]) {
            throw LibsemigroupsException("PartialPerm: duplicate image value "
                                         + std::to_string(val));
          }
          present[val] = true;
        }
      }
    }

    //! A copy constructor.
    PartialPerm<T>(PartialPerm const& copy)
        : PartialTransformation<T, PartialPerm<T>>(copy) {}

    void increase_deg_by(size_t m) override {
      this->_vector.insert(this->_vector.end(), m, UNDEFINED);
      this->reset_hash_value();
    }

    //! Returns \c true if \c this is less than \p that.
    //!
    //! This defines a total order on partial permutations that is equivalent
    //! to that used by GAP. It is not short-lex on the list of images.
    //!
    //! Returns \c true if something complicated is \c true and \c false if
    //! it is not.
    bool operator<(const Element& that) const override {
      auto pp_this = static_cast<const PartialPerm<T>*>(this);
      auto pp_that = static_cast<const PartialPerm<T>&>(that);

      size_t deg_this = pp_this->degree();
      for (auto it = pp_this->_vector.end() - 1; it >= pp_this->_vector.begin();
           it--) {
        if (*it == UNDEFINED) {
          deg_this--;
        } else {
          break;
        }
      }
      size_t deg_that = pp_that.degree();
      for (auto it = pp_that._vector.end() - 1;
           it >= pp_that._vector.begin() && deg_that >= deg_this;
           it--) {
        if (*it == UNDEFINED) {
          deg_that--;
        } else {
          break;
        }
      }

      if (deg_this != deg_that) {
        return deg_this < deg_that;
      }

      for (size_t i = 0; i < deg_this; i++) {
        if ((*pp_this)[i] != pp_that[i]) {
          return (*pp_this)[i] == UNDEFINED
                 || (pp_that[i] != UNDEFINED && (*pp_this)[i] < pp_that[i]);
        }
      }
      return false;
    }

    //! Multiply \p x and \p y and stores the result in \c this.
    //!
    //! See Element::redefine for more details about this method.
    //!
    //! This method asserts that the degrees of \p x, \p y, and \c this, are
    //! all equal, and that neither \p x nor \p y equals \c this.
    void redefine(Element const& x, Element const& y) override {
      LIBSEMIGROUPS_ASSERT(x.degree() == y.degree());
      LIBSEMIGROUPS_ASSERT(x.degree() == this->degree());
      LIBSEMIGROUPS_ASSERT(&x != this && &y != this);
      auto const&  xx = static_cast<PartialPerm<T> const&>(x);
      auto const&  yy = static_cast<PartialPerm<T> const&>(y);
      size_t const n  = this->degree();
      for (T i = 0; i < n; i++) {
        this->_vector[i] = (xx[i] == UNDEFINED ? UNDEFINED : yy[xx[i]]);
      }
      this->reset_hash_value();
    }

    //! Returns the rank of a partial permutation.
    //!
    //! The *rank* of a partial permutation is the number of its distinct image
    //! values, not including PartialTransformation::UNDEFINED. This method
    //! involves slightly less work than PartialTransformation::crank since a
    //! partial permutation is injective, and so every image value occurs
    //! precisely once. This method recomputes the return value every time it
    //! is called.
    size_t crank() const {
      return this->_vector.size()
             - std::count(
                   this->_vector.cbegin(), this->_vector.cend(), UNDEFINED);
    }
  };

  //! Class for bipartitions.
  //!
  //! A *bipartition* is a partition of the set
  //! \f$\{0, ..., 2n - 1\}\f$ for some integer \f$n\f$; see the
  //! [Semigroups package for GAP
  //! documentation](https://gap-packages.github.io/Semigroups/doc/chap3_mj.html)
  //! for more details.

  //! The Bipartition class is more complex (i.e. has more methods) than
  //! strictly required by the algorithms for a Semigroup object because the
  //! extra methods are used in the GAP package [Semigroups package for
  //! GAP](https://gap-packages.github.io/Semigroups/).

  class Bipartition
      : public ElementWithVectorDataDefaultHash<u_int32_t, Bipartition> {
    // TODO add more explanation to the doc here
   public:
    //! A constructor.
    //!
    //! Constructs a uninitialised bipartition of degree \p degree.
    explicit Bipartition(size_t degree)
        : ElementWithVectorDataDefaultHash<u_int32_t, Bipartition>(),
          _nr_blocks(Bipartition::UNDEFINED),
          _nr_left_blocks(Bipartition::UNDEFINED),
          _trans_blocks_lookup(),
          _rank(Bipartition::UNDEFINED) {
      this->_vector.resize(2 * degree);
    }

    //! A constructor.
    //!
    //! The parameter \p blocks must have length *2n* for some positive integer
    //! *n*, consist of non-negative integers, and have the property that if
    //! *i*, *i > 0*, occurs in \p blocks, then *i - 1* occurs earlier in
    //! blocks.  None of this is checked.
    //!
    //! The parameter \p blocks is copied.
    explicit Bipartition(std::vector<u_int32_t> const& blocks)
        : ElementWithVectorDataDefaultHash<u_int32_t, Bipartition>(blocks),
          _nr_blocks(Bipartition::UNDEFINED),
          _nr_left_blocks(Bipartition::UNDEFINED),
          _trans_blocks_lookup(),
          _rank(Bipartition::UNDEFINED) {
      validate();
    }

    Bipartition(std::initializer_list<u_int32_t> blocks)
        : Bipartition(std::vector<u_int32_t>(blocks)) {}

    void validate() const;

    explicit Bipartition(Bipartition const& copy, size_t)
        : ElementWithVectorDataDefaultHash<u_int32_t, Bipartition>(
              copy._vector),
          _nr_blocks(copy._nr_blocks),
          _nr_left_blocks(copy._nr_left_blocks),
          _trans_blocks_lookup(copy._trans_blocks_lookup),
          _rank(copy._rank) {}

    // TODO another constructor that accepts an actual partition

    //! Returns the approximate time complexity of multiplication.
    //!
    //! In the case of a Bipartition of degree *n* the value *2n ^ 2* is
    //! returned.
    size_t complexity() const override;

    //! Returns the degree of the bipartition.
    //!
    //! A bipartition is of degree *n* if it is a partition of
    //! \f$\{0, \ldots, 2n -  1\}\f$.
    size_t degree() const override;

    //! Returns an identity bipartition.
    //!
    //! The *identity bipartition* of degree \f$n\f$ has blocks \f$\{i, -i\}\f$
    //! for all \f$i\in \{0, \ldots, n - 1\}\f$. This method returns a new
    //! identity bipartition of degree equal to the degree of \c this.
    Bipartition identity() const override;

    //! Multiply \p x and \p y and stores the result in \c this.
    //!
    //! This method redefines \c this to be the product (as defined at the top
    //! of this page) of the parameters  \p x and \p y. This method asserts
    //! that the degrees of \p x, \p y, and \c this, are all equal, and that
    //! neither \p x nor  \p y equals \c this.
    //!
    //! The parameter \p thread_id is required since some temporary storage is
    //! required to find the product of \p x and \p y.  Note that if different
    //! threads call this method with the same value of \p thread_id then bad
    //! things will happen.
    void
    redefine(Element const& x, Element const& y, size_t thread_id) override;

    //! Returns the number of transverse blocks.
    //!
    //! The *rank* of a bipartition is the number of blocks containing both
    //! positive and negative values.  This value is cached after it is first
    //! computed.
    size_t rank();

    //! Returns the number of blocks in a bipartition.
    //!
    //! This method differs for Bipartition::nr_blocks in that the number of
    //! blocks is not cached if it has not been previously computed.
    u_int32_t const_nr_blocks() const;

    //! Returns the number of blocks in a bipartition.
    //!
    //! This value is cached the first time it is computed.
    u_int32_t nr_blocks();

    //! Returns the number of blocks containing a positive integer.
    //!
    //! The *left blocks* of a bipartition is the partition of
    //! \f$\{0, \ldots, n - 1\}\f$ induced by the bipartition. This method
    //! returns the number of blocks in this partition.
    u_int32_t nr_left_blocks();

    //! Returns the number of blocks containing a negative integer.
    //!
    //! The *right blocks* of a bipartition is the partition of
    //! \f$\{n, \ldots, 2n - 1\}\f$ induced by the bipartition. This method
    //! returns the number of blocks in this partition.
    u_int32_t nr_right_blocks();

    //! Returns \c true if the block with index \p index is transverse.
    //!
    //! A block of a biparition is *transverse* if it contains integers less
    //! than and greater than \f$n\f$, which is the degree of the bipartition.
    //! This method asserts that the parameter \p index is less than the
    //! number
    //! of blocks in the bipartition.
    bool is_transverse_block(size_t index);

    //! Return the left blocks of a bipartition
    //!
    //! The *left blocks* of a bipartition is the partition of
    //! \f$\{0, \ldots, n - 1\}\f$ induced by the bipartition. This method
    //! returns a Blocks object representing this partition.
    Blocks* left_blocks();

    //! Return the left blocks of a bipartition
    //!
    //! The *right blocks* of a bipartition is the partition of
    //! \f$\{n, \ldots, 2n - 1\}\f$ induced by the bipartition. This method
    //! returns a Blocks object representing this partition.
    Blocks* right_blocks();

    //! Set the cached number of blocks
    //!
    //! This method sets the cached value of the number of blocks of \c this
    //! to \p nr_blocks. It asserts that either there is no existing cached
    //! value or \p nr_blocks equals the existing cached value.
    inline void set_nr_blocks(size_t nr_blocks) {
      LIBSEMIGROUPS_ASSERT(_nr_blocks == Bipartition::UNDEFINED
                           || _nr_blocks == nr_blocks);
      _nr_blocks = nr_blocks;
    }

    //! Set the cached number of left blocks
    //!
    //! This method sets the cached value of the number of left blocks of
    //! \c this to \p nr_left_blocks. It asserts that either there is no
    //! existing cached value or \p nr_left_blocks equals the existing cached
    //! value.
    inline void set_nr_left_blocks(size_t nr_left_blocks) {
      LIBSEMIGROUPS_ASSERT(_nr_left_blocks == Bipartition::UNDEFINED
                           || _nr_left_blocks == nr_left_blocks);
      _nr_left_blocks = nr_left_blocks;
    }

    //! Set the cached rank
    //!
    //! This method sets the cached value of the rank of \c this to \p rank.
    //! It asserts that either there is no existing cached value or
    //! \p rank equals the existing cached value.
    inline void set_rank(size_t rank) {
      LIBSEMIGROUPS_ASSERT(_rank == Bipartition::UNDEFINED || _rank == rank);
      _rank = rank;
    }

   private:
    u_int32_t fuseit(std::vector<u_int32_t>& fuse, u_int32_t pos);
    void      init_trans_blocks_lookup();

    static std::vector<std::vector<u_int32_t>> _fuse;
    static std::vector<std::vector<u_int32_t>> _lookup;

    size_t            _nr_blocks;
    size_t            _nr_left_blocks;
    std::vector<bool> _trans_blocks_lookup;
    size_t            _rank;

    static u_int32_t const UNDEFINED;
  };

  //! Matrices over a semiring.
  //!
  //! This class is abstract since it does not implement all methods required
  //! by the Element class, it exists to provide common methods for its
  //! subclasses.
  //!
  //! The template parameter \p TValueType is the type of the entries in the
  //! matrix, which must also be the type of the corresponding template
  //! parameter for the Semiring object used to define an instance of
  //! a MatrixOverSemiringBase.
  //!
  //! The template parameter \p TSubclass is the type of a subclass of
  //! MatrixOverSemiringBase, and it is used so that it can be passed to
  //! ElementWithVectorData, whose method ElementWithVectorData::identity
  //! returns an instance of \p TSubclass.
  template <typename TValueType, class TSubclass>
  class MatrixOverSemiringBase
      : public ElementWithVectorDataDefaultHash<TValueType, TSubclass> {
   public:
    //! A constructor.
    //!
    //! Constructs a matrix defined by \p matrix.
    //!
    //! The parameter \p matrix should be a vector of integer values of length
    //! \f$n ^ 2\f$ for some integer \f$n\f$, so that the value in position
    //! \f$in + j\f$ is the entry in the \f$i\f$th row and \f$j\f$th column of
    //! the constructed matrix.
    //!
    //! The parameter \p semiring should be a pointer to a Semiring, which
    //! is the semiring over which the matrix is defined.
    //!
    //! This method asserts that the paramater \p semiring is not a nullptr,
    //! that the vector \p matrix has size a non-zero perfect square.
    MatrixOverSemiringBase(std::vector<TValueType> const& matrix,
                           Semiring<TValueType> const*    semiring)
        : ElementWithVectorDataDefaultHash<TValueType, TSubclass>(matrix),
          _degree(sqrt(matrix.size())),
          _semiring(semiring) {
      LIBSEMIGROUPS_ASSERT(semiring != nullptr);
      LIBSEMIGROUPS_ASSERT(!matrix.empty());
      LIBSEMIGROUPS_ASSERT(matrix.size() == _degree * _degree);
    }

    //! A constructor.
    //!
    //! Constructs a matrix defined by \p matrix, which is copied into the
    //! constructed object.
    //!
    //! The parameter \p matrix should be a vector of integer values of length
    //! \f$n ^ 2\f$ for some integer \f$n\f$, so that the value in position
    //! \f$in + j\f$ is the entry in the \f$i\f$th row and \f$j\f$th column of
    //! the constructed matrix.
    //!
    //! The parameter \p semiring should be a pointer to a Semiring, which
    //! is the semiring over which the matrix is defined.
    //!
    //! This method asserts that the paramater \p semiring is not a nullptr,
    //! that the vector \p matrix is not empty, and that every vector contained
    //! in \p matrix has the same length as \p matrix.
    //!
    //! Note that it is the responsibility of the caller to delete the
    //! parameter \p semiring, and that this should only be done after every
    //! MatrixOverSemiringBase object that was constructed using \p semiring
    //! has been deleted.
    MatrixOverSemiringBase(std::vector<std::vector<TValueType>> const& matrix,
                           Semiring<TValueType> const*                 semiring)
        : ElementWithVectorDataDefaultHash<TValueType, TSubclass>(),
          _degree(),
          _semiring(semiring) {
      if (semiring == nullptr) {
        throw LibsemigroupsException("MatrixOverSemiring: semiring is nullptr");
      } else if (matrix.empty()) {
        throw LibsemigroupsException(
            "MatrixOverSemiring: matrix has dimension 0");
      } else if (!all_of(matrix.cbegin(),
                         matrix.cend(),
                         [&matrix](std::vector<TValueType> row) {
                           return row.size() == matrix.size();
                         })) {
        throw LibsemigroupsException(
            "MatrixOverSemiring: matrix is not square");
      }
      _degree = matrix[0].size();
      this->_vector.reserve(matrix.size() * matrix.size());
      for (auto const& row : matrix) {
        this->_vector.insert(this->_vector.end(), row.begin(), row.end());
      }
      validate();
    }

    void validate() const {
      for (auto x : this->_vector) {
        if (!this->_semiring->contains(x)) {
          throw LibsemigroupsException(
              "MatrixOverSemiring: matrix contains entry " + std::to_string(x)
              + " not in the underlying semiring");
        }
      }
    }
    //! A copy constructor.
    //!
    //! The parameter \p increase_deg_by must be 0, since it does not make
    //! sense to increase the degree of a matrix.
    MatrixOverSemiringBase(MatrixOverSemiringBase const& copy)
        : ElementWithVectorDataDefaultHash<TValueType, TSubclass>(copy._vector),
          _degree(copy._degree),
          _semiring(copy._semiring) {}

    // The next constructor only exists to make the empty_key method for
    // ElementWithVectorData work.
#if defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    && defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
    explicit MatrixOverSemiringBase(TValueType)
        : MatrixOverSemiringBase(std::vector<TValueType>()) {}
#endif

    //! Returns a pointer to the Semiring over which the matrix is defined.
    Semiring<TValueType> const* semiring() const {
      return _semiring;
    }

    //! Returns the approximate time complexity of multiplying two matrices.
    //!
    //! The approximate time complexity of multiplying matrices is \f$n ^ 3\f$
    //! where \f$n\f$ is the dimension of the matrix.
    size_t complexity() const override {
      return pow(this->degree(), 3);
    }

    //! Returns the dimension of the matrix.
    //!
    //! The *dimension* of a matrix is just the number of rows (or,
    //! equivalently columns).
    size_t degree() const override {
      return _degree;
    }

    //! Returns the identity matrix with dimension of \c this.
    //!
    //! This method returns a new matrix with dimension equal to that of \c
    //! this, where the main diagonal consists of the value Semiring::one and
    //! every other entry Semiring::zero.
    TSubclass identity() const override {
      std::vector<TValueType> vector;
      vector.resize(this->_vector.size(), _semiring->zero());
      size_t n = this->degree();
      for (auto it = vector.begin(); it < vector.end(); it += n + 1) {
        (*it) = _semiring->one();
      }
      return TSubclass(vector, _semiring);
    }

    //! multiply \p x and \p y and stores the result in \c this.
    //!
    //! this method asserts that the degrees of \p x, \p y, and \c this, are
    //! all equal, and that neither \p x nor \p y equals \c this. it does not
    //! currently verify that \c x, \c y, and \c this are defined over the
    //! same semiring.
    void redefine(Element const& x, Element const& y) override {
      auto xx = static_cast<MatrixOverSemiringBase const&>(x);
      auto yy = static_cast<MatrixOverSemiringBase const&>(y);
      LIBSEMIGROUPS_ASSERT(xx.degree() == yy.degree());
      LIBSEMIGROUPS_ASSERT(xx.degree() == this->degree());
      LIBSEMIGROUPS_ASSERT(&xx != this && &yy != this);
      // It can be that the elements are defined over semirings that are
      // distinct in memory but equal (for example, when one element comes
      // from a semigroup and another from an ideal of that semigroup).
      // LIBSEMIGROUPS_ASSERT(xx->semiring() == yy->semiring() &&
      // xx->semiring() == this->semiring());
      // TODO verify that x, y, and this are defined over the same semiring.
      size_t deg = this->degree();

      for (size_t i = 0; i < deg; i++) {
        for (size_t j = 0; j < deg; j++) {
          int64_t v = _semiring->zero();
          for (size_t k = 0; k < deg; k++) {
            v = _semiring->plus(
                v, _semiring->prod(xx[i * deg + k], yy[k * deg + j]));
          }
          (this->_vector)[i * deg + j] = v;
        }
      }
      after();  // post process this
      this->reset_hash_value();
    }

   protected:
    friend class ElementWithVectorData<TValueType, TSubclass>;
    //! Constructs a MatrixOverSemiringBase with whose underlying semiring is
    //! not defined. The underlying semiring must be set by any class deriving
    //! from this one.
    explicit MatrixOverSemiringBase(std::vector<TValueType> matrix)
        : ElementWithVectorDataDefaultHash<TValueType, TSubclass>(matrix),
          _degree(sqrt(matrix.size())),
          _semiring(nullptr) {}

   private:
    // A function applied after redefinition
    virtual inline void         after() {}
    size_t                      _degree;
    Semiring<TValueType> const* _semiring;
  };

  //! Matrices over a semiring.
  //!
  //! This class is partial specialization of MatrixOverSemiringBase that has
  //! only one template parameter \p TValueType, and that uses
  //! MatrixOverSemiring<TValueType> as the second template parameter for
  //! MatrixOverSemiringBase.
  template <typename TValueType>
  class MatrixOverSemiring
      : public MatrixOverSemiringBase<TValueType,
                                      MatrixOverSemiring<TValueType>> {
    friend class ElementWithVectorData<TValueType,
                                       MatrixOverSemiring<TValueType>>;
    using MatrixOverSemiringBase<TValueType, MatrixOverSemiring<TValueType>>::
        MatrixOverSemiringBase;
  };

  //! Class for projective max-plus matrices.
  //!
  //! These matrices belong to the quotient of the monoid of all max-plus
  //! matrices by the congruence where two matrices are related if they differ
  //! by a scalar multiple. \sa MaxPlusSemiring and MatrixOverSemiring.
  //!
  //! Matrices in this class are modified when constructed to be in a normal
  //! form which is obtained by subtracting the maximum finite entry in the
  //! matrix from the every finite entry.
  class ProjectiveMaxPlusMatrix
      : public MatrixOverSemiringBase<int64_t, ProjectiveMaxPlusMatrix> {
   public:
    //! A constructor.
    //!
    //! See MatrixOverSemiring::MatrixOverSemiring for details about this
    //! method.
    //!
    //! The parameter \p matrix is converted into its normal form when
    //! when the object is constructed.
    ProjectiveMaxPlusMatrix(std::vector<int64_t> const& matrix,
                            Semiring<int64_t> const*    semiring)
        : MatrixOverSemiringBase(matrix, semiring) {
      after();  // this is to put the matrix in normal form
    }

    //! A constructor.
    //!
    //! See MatrixOverSemiring::MatrixOverSemiring for details about this
    //! method.
    //!
    //! The copy of the parameter \p matrix in the object constructed is
    //! converted into its normal form when the object is constructed.
    ProjectiveMaxPlusMatrix(std::vector<std::vector<int64_t>> const& matrix,
                            Semiring<int64_t> const*                 semiring)
        : MatrixOverSemiringBase(matrix, semiring) {
      after();  // this is to put the matrix in normal form
    }

    // The next constructor only exists to make the empty_key method for
    // ElementWithVectorData work.
#if defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    && defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
    explicit ProjectiveMaxPlusMatrix(int64_t x) : MatrixOverSemiringBase(x) {}
#endif

    //! A copy constructor.
    ProjectiveMaxPlusMatrix(ProjectiveMaxPlusMatrix const& copy)
        : MatrixOverSemiringBase(copy) {}

   private:
    friend class ElementWithVectorData<int64_t, ProjectiveMaxPlusMatrix>;
    explicit ProjectiveMaxPlusMatrix(std::vector<int64_t> matrix)
        : MatrixOverSemiringBase(matrix) {}

    // A function applied after redefinition
    inline void after() final {
      int64_t norm = *std::max_element(_vector.begin(), _vector.end());
      for (auto& x : _vector) {
        if (x != LONG_MIN) {
          x -= norm;
        }
      }
    }
  };

  //! Template class for permutations.
  //!
  //! The value of the template parameter \p T can be used to reduce the amount
  //! of memory required by instances of this class; see PartialTransformation
  //! and ElementWithVectorData for more details.
  //!
  //! A *permutation* \f$f\f$ is an injective transformation defined on the
  //! whole of \f$\{0, 1, \ldots, n - 1\}\f$ for some integer \f$n\f$ called the
  //! *degree* of \f$f\f$.
  //! A permutation is stored as a vector of the images of
  //! \f$(0, 1, \ldots, n - 1)\f$,
  //! i.e. \f$((0)f, (1)f, \ldots, (n - 1)f)\f$.
  template <typename T>
  class Permutation : public PartialTransformation<T, Permutation<T>> {
   public:
    using PartialTransformation<T, Permutation<T>>::PartialTransformation;

    void validate() const {
      std::vector<bool> present(this->degree(), false);
      for (auto const& val : this->_vector) {
        if (val < 0 || val >= this->degree()) {
          throw LibsemigroupsException(
              "Permutation: image value out of bounds, found "
              + std::to_string(val) + ", must be less than "
              + std::to_string(this->degree()));
        } else if (present[val]) {
          throw LibsemigroupsException("Permutation: duplicate image value "
                                       + std::to_string(val));
        }
        present[val] = true;
      }
    }

    //! Multiply \p x and \p y and stores the result in \c this.
    //!
    //! See Element::redefine for more details about this method.
    //!
    //! This method asserts that the degrees of \p x, \p y, and \c this, are
    //! all equal, and that neither \p x nor \p y equals \c this.
    // FIXME make a TransformationBase class with this method
    void redefine(Element const& x, Element const& y) override {
      LIBSEMIGROUPS_ASSERT(x.degree() == y.degree());
      LIBSEMIGROUPS_ASSERT(x.degree() == this->degree());
      LIBSEMIGROUPS_ASSERT(&x != this && &y != this);
      auto const&  xx = static_cast<Permutation<T> const&>(x);
      auto const&  yy = static_cast<Permutation<T> const&>(y);
      size_t const n  = this->_vector.size();
      for (T i = 0; i < n; i++) {
        (this->_vector)[i] = yy[xx[i]];
      }
      this->reset_hash_value();
    }

    Permutation identity() const override {
      return this->PartialTransformation<T, Permutation<T>>::identity();
    }

    //! Returns the inverse of a permutation.
    //!
    //! The *inverse* of a permutation \f$f\f$ is the permutation \f$g\f$ such
    //! that \f$fg = gf\f$ is the identity permutation of degree \f$n\f$.
    Permutation inverse() const {
      size_t const n  = this->_vector.size();
      Permutation  id = this->identity();
      for (T i = 0; i < n; i++) {
        id._vector[this->_vector[i]] = i;
      }
      return id;
    }
  };

  //! Matrices over the boolean semiring.
  //!
  //! A *boolean matrix* is a square matrix over the boolean semiring, under
  //! the usual multiplication of matrices.
  //!
  //! Every boolean matrix is defined over a single static copy of the boolean
  //! semiring.
  class BooleanMat : public MatrixOverSemiringBase<bool, BooleanMat> {
   public:
    //! A constructor.
    //!
    //! Constructs a boolean matrix defined by \p matrix.
    //!
    //! The parameter \p matrix should be a vector of boolean values of length
    //! \f$n ^  2\f$ for some integer \f$n\f$, so that the value in position
    //! \f$ni + j\f$ is the entry in row \f$i\f$ and column \f$j\f$ of
    //! the constructed matrix.
    explicit BooleanMat(std::vector<bool> const& matrix)
        : MatrixOverSemiringBase<bool, BooleanMat>(matrix, _semiring) {}

    //! A constructor.
    //!
    //! Constructs a boolean matrix defined by matrix, which is copied into the
    //! constructed boolean matrix; see BooleanMat::BooleanMat for more
    //! details.
    explicit BooleanMat(std::vector<std::vector<bool>> const& matrix)
        : MatrixOverSemiringBase<bool, BooleanMat>(matrix, _semiring) {}

    // The next constructor only exists to make the empty_key method for
    // ElementWithVectorData work.
#if defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    && defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
    explicit BooleanMat(bool x) : MatrixOverSemiringBase(x) {}
#endif

    //! A copy constructor.
    BooleanMat(BooleanMat const& copy)
        : MatrixOverSemiringBase<bool, BooleanMat>(copy._vector,
                                                   copy._semiring) {}

    //! Multiplies \p x and \p y and stores the result in \c this.
    //!
    //! This method asserts that the dimensions of \p x, \p y, and \c this, are
    //! all equal, and that neither \p x nor \p y equals \c this.
    void redefine(Element const& x, Element const& y);

    // There is a specialization of std::hash for std::vector<bool> but for
    // some reason it causes a dramatic slow down in some of the benchmarks if
    // it is used by cache_hash_value below. So, we leave this commented out as
    // a reminder not to use it.

    // protected:
    // void cache_hash_value() const override {
    //  this->_hash_value = std::hash<std::vector<bool>>{}(this->_vector);
    // }

   private:
    // The next constructor only exists to allow the identity method for
    // MatrixOverSemiringBase to work.
    friend class MatrixOverSemiringBase<bool, BooleanMat>;
    BooleanMat(std::vector<bool> matrix, Semiring<bool> const* semiring)
        : MatrixOverSemiringBase<bool, BooleanMat>(matrix, semiring) {}

    static BooleanSemiring const* const _semiring;
  };

  //! Class for partitioned binary relations (PBR).
  //!
  //! Partitioned binary relations (PBRs) are a generalisation of bipartitions,
  //! which were introduced by
  //! [Martin and Mazorchuk](https://arxiv.org/abs/1102.0862).
  class PBR : public ElementWithVectorData<std::vector<u_int32_t>, PBR> {
   public:
    //! Constructs a PBR defined by the vector pointed to by \p vector.
    //!
    //! The parameter \p vector should be a pointer to a vector of vectors of
    //! non-negative integer values of length \f$2n\f$ for some integer
    //! \f$n\f$, the vector in position \f$i\f$ is the list of points adjacent
    //! to \f$i\f$ in the PBR.
    using ElementWithVectorData<std::vector<u_int32_t>,
                                PBR>::ElementWithVectorData;

    explicit PBR(std::initializer_list<std::vector<u_int32_t>> vec);

    // TODO add a constructor like the one in GAP

    void validate() const;

    //! Returns the approximate time complexity of multiplying PBRs.
    //!
    //! The approximate time complexity of multiplying PBRs is \f$2n ^ 3\f$
    //! where \f$n\f$ is the degree.
    size_t complexity() const override;

    //! Returns the degree of a PBR.
    //!
    //! The *degree* of a PBR is half the number of points in the PBR, which is
    //! also half the length of the underlying vector
    //! ElementWithVectorData::_vector.
    size_t degree() const override;

    //! Returns the identity PBR with degree equal to that of \c this.
    //!
    //! This method returns a new PBR with degree equal to the degree of \c
    //! this where every value is adjacent to its negative. Equivalently,
    //! \f$i\f$ is adjacent \f$i + n\f$ and vice versa for every \f$i\f$ less
    //! than the degree \f$n\f$.
    PBR identity() const override;

    //! Multiply \p x and \p y and stores the result in \c this.
    //!
    //! This method redefines \c this to be the product
    //! of the parameters  \p x and \p y. This method asserts
    //! that the degrees of \p x, \p y, and \c this, are all equal, and that
    //! neither \p x nor  \p y equals \c this.
    //!
    //! The parameter \p thread_id is required since some temporary storage is
    //! required to find the product of \p x and \p y.  Note that if different
    //! threads call this method with the same value of \p thread_id then bad
    //! things will happen.
    void redefine(Element const&, Element const&, size_t) override;

#if defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    && defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
    Element* empty_key() const override {
      return new PBR({{ std::numeric_limits<u_int32_t>::max() }});
    }
#endif

   protected:
    void cache_hash_value() const override;

   private:
    void unite_rows(RecVec<bool>& out,
                    RecVec<bool>& tmp,
                    size_t const& vertex1,
                    size_t const& vertex2);

    void x_dfs(std::vector<bool>& x_seen,
               std::vector<bool>& y_seen,
               RecVec<bool>&      tmp,
               u_int32_t const&   n,
               u_int32_t const&   i,
               PBR const* const   x,
               PBR const* const   y,
               size_t const&      adj);

    void y_dfs(std::vector<bool>& x_seen,
               std::vector<bool>& y_seen,
               RecVec<bool>&      tmp,
               u_int32_t const&   n,
               u_int32_t const&   i,
               PBR const* const   x,
               PBR const* const   y,
               size_t const&      adj);

    static std::vector<std::vector<bool>> _x_seen;
    static std::vector<std::vector<bool>> _y_seen;
    static std::vector<RecVec<bool>>      _out;
    static std::vector<RecVec<bool>>      _tmp;
  };

  // template <typename T, typename... Args>
  // T inline make_element(Args&&... args) {
  //   T res(std::forward<Args>(args)...);
  //   res.validate();
  //   return res;
  // }

  template <size_t N> struct SmallestInteger {
    using type = typename std::conditional<
        N >= 0x100000000,
        u_int64_t,
        typename std::conditional<
            N >= 0x10000,
            u_int32_t,
            typename std::conditional<N >= 0x100, u_int16_t, u_int8_t>::type>::
            type>::type;
  };

  template <size_t N> struct Transf {
#ifdef LIBSEMIGROUPS_USE_HPCOMBI
    using type = typename std::conditional<
        N >= 16,
        Transformation<typename SmallestInteger<N>::type>,
        HPCombi::Transf16>::type;
#else
    using type = Transformation<typename SmallestInteger<N>::type>;
#endif
  };

  template <size_t N> struct PPerm {
#ifdef LIBSEMIGROUPS_USE_HPCOMBI
    using type = typename std::conditional<
        N >= 16,
        PartialPerm<typename SmallestInteger<N>::type>,
        HPCombi::PTransf16>::type;
#else
    using type = PartialPerm<typename SmallestInteger<N>::type>;
#endif
  };

  template <size_t N> struct Perm {
#ifdef LIBSEMIGROUPS_USE_HPCOMBI
    using type = typename std::conditional<
        N >= 16,
        Permutation<typename SmallestInteger<N>::type>,
        HPCombi::Perm16>::type;
#else
    using type = Permutation<typename SmallestInteger<N>::type>;
#endif
  };

  class BMat8;
  template <size_t N> struct BMat {
    using type = typename std::conditional<N >= 9, BooleanMat, BMat8>::type;
  };
}  // namespace libsemigroups

namespace std {
  //! Provides a call operator returning a hash value for an Element
  //! via a pointer.
  //!
  //! This struct provides a call operator for obtaining a hash value for the
  //! Element from a const Element pointer. This is used by various methods
  //! of the Semigroup class.
  template <> struct hash<libsemigroups::Element*> {
    size_t operator()(libsemigroups::Element* x) const {
      return x->hash_value();
    }
  };
  template <> struct hash<libsemigroups::Element const*> {
    size_t operator()(libsemigroups::Element const* x) const {
      return x->hash_value();
    }
  };
  // FIXME the next struct should be deleted
  template <> struct hash<libsemigroups::Element> {
    size_t operator()(libsemigroups::Element const& x) const {
      return x.hash_value();
    }
  };

  //! Provides a call operator for comparing Elements via pointers.
  //!
  //! This struct provides a call operator for comparing const Element
  //! pointers (by comparing the Element objects they point to). This is used
  //! by various methods of the Semigroup class.
  template <> struct equal_to<libsemigroups::Element*> {
    bool operator()(libsemigroups::Element* x,
                    libsemigroups::Element* y) const {
      return *x == *y;
    }
  };
  template <> struct equal_to<libsemigroups::Element const*> {
    bool operator()(libsemigroups::Element const* x,
                    libsemigroups::Element const* y) const {
      return *x == *y;
    }
  };

  template <typename TIntegerType>
  struct hash<libsemigroups::Transformation<TIntegerType>> {
    size_t
    operator()(libsemigroups::Transformation<TIntegerType> const& x) const {
      return x.hash_value();
    }
  };

}  // namespace std
#endif  // LIBSEMIGROUPS_SRC_ELEMENTS_H_
