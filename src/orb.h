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

  template <typename ElementType,
            typename PointType,
            typename Hasher,
            typename Equaliser>
  class Orb {
   public:
    typedef std::function<PointType(ElementType*, PointType, PointType)>
                                                action_type;
    typedef std::function<PointType(PointType)> copier_type;
    typedef PointType                           point_type;
    typedef ElementType                         element_type;

    Orb() { }

    Orb(std::vector<ElementType*> gens,
        PointType                 seed,
        action_type                    act,
        copier_type                    copier = default_copier)
        : _act(act),
          _copier(copier),
          _gens(gens),
          _map({std::make_pair(seed, 0)}),
          _orb({seed}),
          _tmp_point(copier(seed)) {
      LIBSEMIGROUPS_ASSERT(!gens.empty());
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
          }
        }
      }
    }

    /*    // TODO change the name to find
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
        }*/

    size_t size() {
      enumerate();
      return _orb.size();
    }

   private:
    static PointType default_copier(PointType pt) {
      return PointType(pt);
    }

    action_type                    _act;
    copier_type                    _copier;
    std::vector<ElementType*> _gens;
    std::unordered_map<PointType, size_t, Hasher, Equaliser> _map;
    std::vector<PointType> _orb;

    std::vector<ElementType*> _gen;
    std::vector<size_t>       _parent;
    PointType                 _tmp_point;
  };
}  // namespace libsemigroups

#endif  // LIBSEMIGROUPS_SRC_ORB_H_
