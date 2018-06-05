//
// libsemigroups - C++ library for semigroups and monoids
// Copyright (C) 2018 James D. Mitchell
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

// This file contains the declaration of an element container, i.e. an object
// that contains some kind of element that can be multiplied etc.

#ifndef LIBSEMIGROUPS_SRC_ELTCONT_H_
#define LIBSEMIGROUPS_SRC_ELTCONT_H_

#include "elements.h"

namespace libsemigroups {

  template <typename TElementType, typename = void> struct ElementContainer {};

  // Specialization for trivial, non-pointer element types, such as BMat8,
  // Transf16 and so on . . .
  template <typename TElementType>
  struct ElementContainer<
      TElementType,
      typename std::enable_if<!std::is_pointer<TElementType>::value
                              && std::is_trivial<TElementType>::value>::type> {
    using value_type       = TElementType;
    using const_value_type = TElementType const;
    using reference        = TElementType&;
    using const_reference  = TElementType const&;

    using internal_value_type       = value_type;
    using internal_const_value_type = const_value_type;
    using internal_reference        = reference;
    using internal_const_reference  = const_reference;

    internal_value_type to_internal(const_reference x) const {
      return x;
    }

    value_type to_external(const_reference x) const {
      return x;
    }

    inline void multiply(internal_reference       xy,
                         internal_const_reference x,
                         internal_const_reference y,
                         size_t = 0) const {
      xy = x * y;
    }

    inline internal_value_type copy(internal_const_reference x) const {
      return x;
    }

    inline void increase_deg_by(size_t = 0) const {}

    inline void free(internal_value_type) const {}

    inline void swap(internal_reference x, internal_reference y) const {
      std::swap(x, y);
    }

    inline internal_value_type one(internal_const_reference x) const {
      return x.one();
    }

    inline size_t element_degree(internal_const_reference) const {
      return 0;
    }

    inline size_t complexity(internal_const_reference) const {
      return 1;
    }

    inline bool cmp(internal_const_reference x,
                    internal_const_reference y) const {
      return x < y;
    }

#if defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    && defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
    // TODO this could be static
    inline internal_value_type empty_key(internal_const_reference) const {
      return internal_value_type(-1);
    }
#endif
  };

  // Specialization for pointer element types, such as Element* and so on . . .
  template <typename TElementType>
  struct ElementContainer<
      TElementType,
      typename std::enable_if<std::is_pointer<TElementType>::value>::type> {
    using value_type = TElementType;
    using const_value_type =
        typename std::remove_pointer<value_type>::type const*;
    using reference       = value_type;
    using const_reference = const_value_type;

    using internal_value_type       = value_type;
    using internal_const_value_type = const_value_type;

    internal_value_type to_internal(value_type x) const {
      return x;
    }

    internal_const_value_type to_internal(const_value_type x) const {
      return x;
    }

    value_type to_external(internal_value_type x) const {
      return x;
    }

    const_value_type to_external(internal_const_value_type x) const {
      return x;
    }

    inline void free(internal_value_type x) const {
      delete x;
    }

    internal_value_type copy(internal_const_value_type x) const {
      return dynamic_cast<internal_value_type>(x->heap_copy());
    }

    inline void increase_deg_by(internal_value_type x, size_t m) const {
      x->increase_deg_by(m);
    }

    inline internal_value_type one(internal_const_value_type x) const {
      return dynamic_cast<internal_value_type>(x->heap_identity());
    }

    inline void multiply(internal_value_type       xy,
                         internal_const_value_type x,
                         internal_const_value_type y,
                         size_t                    tid = 0) const {
      xy->Element::redefine(*x, *y, tid);
    }

    inline void swap(internal_value_type x, internal_value_type y) const {
      x->swap(*y);
    }

    inline size_t element_degree(internal_const_value_type x) const {
      return x->degree();
    }

    inline size_t complexity(internal_const_value_type x) const {
      return x->complexity();
    }

    inline bool cmp(internal_const_value_type x,
                    internal_const_value_type y) const {
      return *x < *y;
    }

#if defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    && defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
    // TODO could be static
    inline internal_value_type empty_key(internal_const_value_type x) const {
      return x->empty_key();
    }
#endif
  };

  // Specialization for non-pointer non-trivial element types, such as
  // Transformation<size_t> and so on . . .
  template <typename TElementType>
  struct ElementContainer<
      TElementType,
      typename std::enable_if<!std::is_trivial<TElementType>::value>::type> {
    static_assert(!std::is_pointer<TElementType>::value,
                  "TElementType must not be a pointer");
    using value_type       = TElementType;
    using const_value_type = TElementType const;
    using reference        = TElementType&;
    using const_reference  = TElementType const&;

    using internal_value_type       = TElementType*;
    using internal_const_value_type = TElementType const*;

    internal_value_type to_internal(reference x) const {
      return &x;
    }

    internal_const_value_type to_internal(const_reference x) const {
      return &x;
    }

    reference to_external(internal_value_type x) const {
      return *x;
    }

    const_reference to_external(internal_const_value_type x) const {
      return *x;
    }

    inline void free(internal_value_type x) const {
      delete x;
    }

    inline internal_value_type copy(internal_const_value_type x) const {
      return dynamic_cast<internal_value_type>(x->heap_copy());
    }

    inline void increase_deg_by(internal_value_type x, size_t m) const {
      x->increase_deg_by(m);
    }

    inline internal_value_type one(internal_const_value_type x) const {
      return dynamic_cast<internal_value_type>(x->heap_identity());
    }

    inline void multiply(internal_value_type       xy,
                         internal_const_value_type x,
                         internal_const_value_type y,
                         size_t                    tid = 0) const {
      xy->Element::redefine(*x, *y, tid);
    }

    inline void swap(internal_value_type x, internal_value_type y) const {
      x->swap(*y);
    }

    inline size_t element_degree(internal_const_value_type x) const {
      return x->degree();
    }

    inline size_t complexity(internal_const_value_type x) const {
      return x->complexity();
    }

    inline bool cmp(internal_const_value_type x,
                    internal_const_value_type y) const {
      return *x < *y;
    }

#if defined(LIBSEMIGROUPS_HAVE_DENSEHASHMAP) \
    && defined(LIBSEMIGROUPS_USE_DENSEHASHMAP)
    // TODO could be static
    inline internal_value_type empty_key(internal_const_value_type x) const {
      return x->empty_key();
    }
#endif
  };
}  // namespace libsemigroups

#endif  // LIBSEMIGROUPS_SRC_ELTCONT_H_
