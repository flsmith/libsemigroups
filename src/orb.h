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

// This file contains ...

#ifndef LIBSEMIGROUPS_SRC_ORB_H_
#define LIBSEMIGROUPS_SRC_ORB_H_

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "semigroups.h"
#include "timer.h"

namespace libsemigroups {
  namespace internal {
    template <typename T> struct Equal {
      bool operator()(T i, T j) const {
        return i == j;
      }
    };
    template <typename T> struct Hash {
      size_t operator()(T i) const {
        return std::hash<T>()(i);
      }
    };
  }  // namespace internal

  template <typename ElementType,
            typename PointType,
            typename Hasher    = internal::Hash<PointType>,
            typename Equaliser = internal::Equal<PointType>>
  class Orb {
   public:
    typedef std::function<PointType(ElementType*, PointType, PointType)>
                                                action_type;
    typedef std::function<PointType(PointType)> copier_type;

    static const size_t UNDEFINED;

    Orb(std::vector<ElementType*> gens,
        PointType                 seed,
        action_type               act,
        copier_type               copier = default_reference_copier)
        : _act(act),
          _copier(copier),
          _gen({nullptr}),
          _gens(gens),
          _map({std::make_pair(seed, 0)}),
          _orb({seed}),
          _parent({UNDEFINED}),
          _tmp_point(copier(seed)) {
      LIBSEMIGROUPS_ASSERT(!gens.empty());
      _tmp_element = static_cast<ElementType*>(_gens[0]->really_copy());
    }

    ~Orb() {}

    void enumerate() {
      for (size_t i = 0; i < _orb.size(); i++) {
        for (ElementType* x : _gens) {
          _tmp_point = _act(x, _orb[i], _tmp_point);
          if (_map.find(_tmp_point) == _map.end()) {
            PointType pt = _copier(_tmp_point);
            _map.insert(std::make_pair(pt, _orb.size()));
            _orb.push_back(pt);
            _gen.push_back(x);
            _parent.push_back(i);
          }
        }
      }
    }

    size_t position(PointType pt) {
      auto it = _map.find(pt);
      if (it != _map.end()) {
        return (*it).second;
      } else {
        return UNDEFINED;
      }
    }

    typename std::vector<PointType>::const_iterator find(PointType pt) {
      auto it = _map.find(pt);
      if (it != _map.end()) {
        return _orb.cbegin() + (*it).second;
      } else {
        return _orb.cend();
      }
    }

    void reserve(size_t n) {
      _map.reserve(n);
      _orb.reserve(n);
      _gen.reserve(n);
      _parent.reserve(n);
    }

    inline PointType operator[](size_t pos) const {
      assert(pos < _orb.size());
      return _orb[pos];
    }

    inline PointType at(size_t pos) const {
      return _orb.at(pos);
    }

    size_t size() {
      enumerate();
      return _orb.size();
    }

    ElementType* mapper(size_t pos) {
      ElementType* out = static_cast<ElementType*>(_gen[pos]->really_copy());
      pos              = _parent[pos];
      while (pos != 0) {
        _tmp_element->redefine(_gen[pos], out);
        out->swap(_tmp_element);
        pos = _parent[pos];
      }
      return out;
    }

   private:
    static PointType default_reference_copier(PointType pt) {
      return PointType(pt);
    }

    action_type _act;
    copier_type _copier;

    std::vector<ElementType*> _gen;
    std::vector<ElementType*> _gens;
    std::unordered_map<PointType, size_t, Hasher, Equaliser> _map;
    std::vector<PointType> _orb;

    std::vector<size_t> _parent;
    ElementType*        _tmp_element;
    PointType           _tmp_point;
  };

  template <typename ElementType,
            typename PointType,
            typename Hasher,
            typename Equaliser>
  size_t const Orb<ElementType, PointType, Hasher, Equaliser>::UNDEFINED
      = std::numeric_limits<size_t>::max();
}  // namespace libsemigroups

#endif  // LIBSEMIGROUPS_SRC_ORB_H_
