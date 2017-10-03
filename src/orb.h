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
        action_type               act,
        copier_type               copier = default_reference_copier)
        : _act(act),
          _copier(copier),
          _enumerated(false),
          _gen(),
          _gens(gens),
          _map(),
          _orb(),
          _parent() {
      LIBSEMIGROUPS_ASSERT(!gens.empty());
      _tmp_element = static_cast<ElementType*>(_gens[0]->really_copy());
    }

    Orb(std::vector<ElementType*> gens,
        PointType                 seed,
        action_type               act,
        copier_type               copier = default_reference_copier)
        : Orb(gens, act, copier) {
      add_seed(seed);
    }

    ~Orb() {  // TODO delete pts it they are pointers
    }

    // Returns the position the seed was added at.
    size_t add_seed(PointType seed) {
      LIBSEMIGROUPS_ASSERT(empty() || is_done());
      LIBSEMIGROUPS_ASSERT(_map.find(seed) == _map.end());
      _tmp_point = _copier(seed);
      _gen.push_back(nullptr);
      _map.emplace(std::make_pair(seed, 0));
      _orb.push_back(seed);
      _parent.push_back(UNDEFINED);
      _enumerated = false;
      return _orb.size() - 1;
    }

    void enumerate() {
      if (_enumerated) {
        return;
      }
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
      _enumerated = true;
    }

    bool is_done() const {
      return _enumerated;
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

    bool empty() const {
      return _orb.empty();
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

   protected:
    static PointType default_reference_copier(PointType pt) {
      return PointType(pt);
    }

    action_type               _act;
    copier_type               _copier;
    bool                      _enumerated;
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
            typename Hasher    = internal::Hash<PointType>,
            typename Equaliser = internal::Equal<PointType>>
  class GradedOrb : public Orb<ElementType, PointType, Hasher, Equaliser> {
   public:
    typedef Orb<ElementType, PointType, Hasher, Equaliser> orb_base_type;
    typedef std::function<size_t(PointType)>    grade_type;
    typedef typename orb_base_type::action_type action_type;
    typedef typename orb_base_type::copier_type copier_type;

    GradedOrb(std::vector<ElementType*> gens,
              action_type               act,
              copier_type               copier,
              grade_type                grader)
        : orb_base_type(gens, act, copier), _grader(grader) {}

    void enumerate(PointType seed) {
      size_t pos   = this->add_seed(seed);
      size_t grade = _grader(seed);
      LIBSEMIGROUPS_ASSERT(this->empty() || grade == _grader(this->at(0)));
      for (size_t i = pos; i < this->_orb.size(); ++i) {
        for (ElementType* x : this->_gens) {
          this->_tmp_point = this->_act(x, this->_orb[i], this->_tmp_point);
          if (_grader(this->_tmp_point) == grade
              && this->find(this->_tmp_point) == this->_orb.end()) {
            PointType pt = this->_copier(this->_tmp_point);
            this->_map.insert(std::make_pair(pt, this->_orb.size()));
            this->_orb.push_back(pt);
            this->_gen.push_back(x);
            this->_parent.push_back(i);
          }
        }
      }
      this->_enumerated = true;
    }

   private:
    grade_type _grader;
  };

  template <typename ElementType,
            typename PointType,
            typename Hasher,
            typename Equaliser>
  size_t const Orb<ElementType, PointType, Hasher, Equaliser>::UNDEFINED
      = std::numeric_limits<size_t>::max();
}  // namespace libsemigroups

#endif  // LIBSEMIGROUPS_SRC_ORB_H_
