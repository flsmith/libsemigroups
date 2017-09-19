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

  template <typename ElementType, typename PointType> class Orb {
   public:
    Orb(std::vector<ElementType*> gens, PointType seed)
        : _gens(gens), _orb({seed}), _map({std::make_pair(seed, 0)}) {
       _gen({nullptr}),
       _parent({-1}) {
      LIBSEMIGROUPS_ASSERT(!gens.empty());
    }

    ~Orb();

    void enumerate() {
      for (size_t i = 0; i < _orb.size(); i++) {
        for (ElementType const* x : _gens) {
          PointType pt = (*x)[_orb[i]];
          if (_map.find(pt) == _map.end()) {
            _map.insert(std::make_pair(pt, _orb.size()));
            _orb.push_back(pt);
            _gen.push_back(x);
            _parent.push_back(i);
          }
        }
      }
    }

    // TODO change the name to find
    size_t position(PointType pt) {
      auto it = _map.find(pt);
      if (it != _map.end()) {
        return (*it).second;
      } else {
        return -1;
      }
    }

    void reserve(size_t n) {
      _map.reserve(n);
      _orb.reserve(n);
      _gen.reserve(n);
    }

    inline PointType operator[](size_t pos) const {
      assert(pos < _orb.size());
      return _orb[pos];
    }

    inline PointType at(size_t pos) const {
      return _orb.at(pos);
    }

    size_t size() {
      return _orb.size();
    }

   private:
    std::vector<ElementType*> _gens;
    std::unordered_map<PointType, size_t> _map;
    std::vector<PointType>    _orb;
    std::vector<ElementType*> _gen;
    std::vector<size_t>       _parent;
    std::vector<ElementType*> _mappers;
    static ElementType*       _tmp
        = new Permutation<PointType>(new std::vector<PointType>());
  };
}  // namespace libsemigroups

#endif  // LIBSEMIGROUPS_SRC_ORB_H_
