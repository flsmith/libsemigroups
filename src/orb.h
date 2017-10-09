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
#include <numeric>
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
          _gens(gens),
          _map(),
          _orb() {
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

    virtual ~Orb() {  // TODO delete pts it they are pointers
    }

    // Returns the position the seed was added at.
    virtual size_t add_seed(PointType seed) {
      //LIBSEMIGROUPS_ASSERT(empty() || is_done());
      if (_map.find(seed) == _map.end()) {
        _tmp_point = _copier(seed);
        _map.emplace(std::make_pair(seed, _orb.size()));
        _orb.push_back(seed);
        _enumerated = false;
        return _orb.size() - 1;
      }
      return UNDEFINED;
    }

    void enumerate() {
      if (_enumerated) {
        return;
      }
      for (size_t i = 0; i < _orb.size(); i++) {
        for (ElementType* x : _gens) {
          _tmp_point = _act(x, _orb[i], _tmp_point);
          if (is_new_point()) {
            process_new_point(x, i);
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

    virtual void reserve(size_t n) {
      _map.reserve(n);
      _orb.reserve(n);
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

    size_t current_size() const {
      return _orb.size();
    }

   protected:
    virtual inline bool is_new_point() {
      return _map.find(_tmp_point) == _map.end();
    }

    virtual inline void process_new_point(ElementType* x, size_t i) {
      (void) x;
      (void) i;
      PointType pt = _copier(_tmp_point);
      _map.insert(std::make_pair(pt, _orb.size()));
      _orb.push_back(pt);
    }

    static PointType default_reference_copier(PointType pt) {
      return PointType(pt);
    }

    action_type               _act;
    copier_type               _copier;
    bool                      _enumerated;
    std::vector<ElementType*> _gens;
    std::unordered_map<PointType, size_t, Hasher, Equaliser> _map;
    std::vector<PointType> _orb;

    ElementType* _tmp_element;
    PointType    _tmp_point;
  };

  template <typename ElementType,
            typename PointType,
            typename Hasher,
            typename Equaliser>
  size_t const Orb<ElementType, PointType, Hasher, Equaliser>::UNDEFINED
      = std::numeric_limits<size_t>::max();

  template <typename ElementType,
            typename PointType,
            typename Hasher    = internal::Hash<PointType>,
            typename Equaliser = internal::Equal<PointType>>
  class OrbWithTree : public Orb<ElementType, PointType, Hasher, Equaliser> {
   public:
    typedef Orb<ElementType, PointType, Hasher, Equaliser> orb_base_type;
    typedef typename orb_base_type::action_type action_type;
    typedef typename orb_base_type::copier_type copier_type;
    using orb_base_type::UNDEFINED;

    OrbWithTree(std::vector<ElementType*> gens,
                action_type               act,
                copier_type               copier)
        : orb_base_type(gens, act, copier), _gen(), _parent() {}

    OrbWithTree(std::vector<ElementType*> gens,
                PointType                 seed,
                action_type               act,
                copier_type               copier)
        : OrbWithTree(gens, act, copier) {
      this->add_seed(seed);
    }

    size_t add_seed(PointType seed) override {
      size_t out = orb_base_type::add_seed(seed);
      _gen.push_back(nullptr);
      _parent.push_back(UNDEFINED);
      return out;
    }

    void reserve(size_t n) override {
      orb_base_type::reserve(n);
      _gen.reserve(n);
      _parent.reserve(n);
    }

    ElementType* mapper(size_t pos) {
      ElementType* out = static_cast<ElementType*>(_gen[pos]->really_copy());
      pos              = _parent[pos];
      while (pos != 0) {
        this->_tmp_element->redefine(_gen[pos], out);
        out->swap(this->_tmp_element);
        pos = _parent[pos];
      }
      return out;
    }

   protected:
    inline void process_new_point(ElementType* x, size_t i) override {
      orb_base_type::process_new_point(x, i);
      _gen.push_back(x);
      _parent.push_back(i);
    }

    std::vector<ElementType*> _gen;
    std::vector<size_t>       _parent;
  };

  template <typename ElementType,
            typename PointType,
            typename Hasher    = internal::Hash<PointType>,
            typename Equaliser = internal::Equal<PointType>>
  class GradedOrb
      : public OrbWithTree<ElementType, PointType, Hasher, Equaliser> {
   public:
    typedef OrbWithTree<ElementType, PointType, Hasher, Equaliser>
                                                orb_base_type;
    typedef std::function<size_t(PointType)>    grade_type;
    typedef typename orb_base_type::action_type action_type;
    typedef typename orb_base_type::copier_type copier_type;
    using orb_base_type::UNDEFINED;

    GradedOrb(std::vector<ElementType*> gens,
              action_type               act,
              copier_type               copier,
              grade_type                grader)
        : orb_base_type(gens, act, copier),
          _grader(grader),
          _grade(UNDEFINED) {}

    size_t add_seed(PointType seed) override {
      if (_grade == UNDEFINED || _grader(seed) == _grade) {
        size_t out = orb_base_type::add_seed(seed);
        _grade = _grader(seed);
        return out;
      }
      return UNDEFINED;
    }

    typename std::unordered_set<PointType>::const_iterator
      begin_low_grade_points() const {
        return _low_grade_points.cbegin();
    }

    typename std::unordered_set<PointType>::const_iterator
      end_low_grade_points() const {
        return _low_grade_points.cend();
    }

    void set_grade(size_t val) {
      _grade = val;
    }

   private:
    inline void process_new_point(ElementType* x, size_t i) override {
      if (_grader(this->_tmp_point) == _grade) {
        orb_base_type::process_new_point(x, i);
      } else {
        LIBSEMIGROUPS_ASSERT(_grader(this->_tmp_point) < _grade);
        _low_grade_points.insert(this->_copier(this->_tmp_point));
      }
    }
    grade_type _grader;
    size_t     _grade;
    std::unordered_set<PointType> _low_grade_points;
    // TODO keep these in GradedOrbs not here!
  };

  template <typename ElementType,
            typename PointType,
            typename Hasher    = internal::Hash<PointType>,
            typename Equaliser = internal::Equal<PointType>>
  class GradedOrbs {
   public:
    typedef GradedOrb<ElementType, PointType, Hasher, Equaliser> orb_type;
    typedef std::function<size_t(PointType)>    grade_type;
    typedef typename orb_type::action_type action_type;
    typedef typename orb_type::copier_type copier_type;

    GradedOrbs(std::vector<ElementType*> gens,
               PointType                 seed,
               action_type               act,
               copier_type               copier,
               grade_type                grader)
        : _act(act),
          _copier(copier),
          _gens(gens),
          _grader(grader),
          _seed(seed),
          _ungraded_orb(gens, act, copier, [this](PointType pt) -> size_t {
            return (this->position(pt) == UNDEFINED ? 1 : 0);
          }) {
          }

    void add_seed(PointType seed) {
      auto it = _graded_orbs_map.find(_grader(seed));
      if (it == _graded_orbs_map.end()) {
        orb_type o(_gens, _act, _copier, _grader);
        o.add_seed(seed);
        _graded_orbs_map.insert(std::make_pair(_grader(seed), o));
      } else if ((*it).second.position(seed) == orb_type::UNDEFINED) {
        (*it).second.add_seed(seed);
      }
    }

    size_t size() {
      size_t out = 0;
      for (auto & o : _graded_orbs_map) {
        out += o.second.size();
      }
      enumerate();
      out += _ungraded_orb.size();
      return out;
    }

    size_t current_size() {
      size_t out = 0;
      for (auto & o : _graded_orbs_map) {
        out += o.second.size();
      }
      out += _ungraded_orb.current_size();
      return out;
    }

    std::pair<size_t, size_t> position(PointType pt) {
      auto it = _graded_orbs_map.find(_grader(pt));
      if (it == _graded_orbs_map.end()) {
        return UNDEFINED;
      }
      size_t pos = (*it).second.position(pt);
      if (pos == orb_type::UNDEFINED) {
        return UNDEFINED;
      }
      return std::make_pair((*it).first, pos);
    }

    void enumerate() {
      _ungraded_orb.set_grade(1);
      _ungraded_orb.add_seed(_seed);
      for (auto pair : _graded_orbs_map) {
        for (auto it = pair.second.begin_low_grade_points();
             it != pair.second.end_low_grade_points();
             ++it) {
          _ungraded_orb.add_seed(*it);
        }
      }
      _ungraded_orb.enumerate();
    }

    static const std::pair<size_t, size_t> UNDEFINED;

   private:
    action_type               _act;
    copier_type               _copier;
    std::vector<ElementType*> _gens;
    grade_type                _grader;
    std::unordered_map<size_t, orb_type> _graded_orbs_map;
    PointType                 _seed;
    orb_type                  _ungraded_orb;
  };

  template <typename ElementType,
            typename PointType,
            typename Hasher,
            typename Equaliser>
  std::pair<size_t, size_t> const
      GradedOrbs<ElementType, PointType, Hasher, Equaliser>::UNDEFINED
      = std::make_pair(std::numeric_limits<size_t>::max(),
                       std::numeric_limits<size_t>::max());

}  // namespace libsemigroups

#endif  // LIBSEMIGROUPS_SRC_ORB_H_
