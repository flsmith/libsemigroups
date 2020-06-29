//
// libsemigroups - C++ library for semigroups and monoids
// Copyright (C) 2019 Finn Smith
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
// This file contains an implementation of Konieczny's algorithm for computing
// subsemigroups of the boolean matrix monoid.

// TODO: exception safety!

#ifndef LIBSEMIGROUPS_INCLUDE_KONIECZNY_HPP_
#define LIBSEMIGROUPS_INCLUDE_KONIECZNY_HPP_

#include <algorithm>
#include <map>
#include <set>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "action.hpp"
#include "bmat8.hpp"
#include "constants.hpp"
#include "digraph.hpp"
#include "element.hpp"
#include "schreier-sims.hpp"

//
namespace libsemigroups {
  namespace bmat8_helpers {

    size_t min_possible_dim(BMat8 const& x) {
      size_t i = 1;
      size_t d = x.to_int();
      size_t y = x.transpose().to_int();

      while ((d >> (8 * i)) << (8 * i) == d && (y >> (8 * i)) << (8 * i) == y
             && i < 9) {
        ++i;
      }
      return 9 - i;
    }

  }  // namespace bmat8_helpers

  namespace konieczny_helpers {
    template <typename TElementType>
    bool is_group_index(TElementType const& x, TElementType const& y) {
      LIBSEMIGROUPS_ASSERT(Rho<TElementType>()(x) == x
                           && Lambda<TElementType>()(y) == y);
      return Lambda<TElementType>()(y * x) == Lambda<TElementType>()(x)
             && Rho<TElementType>()(y * x) == Rho<TElementType>()(y);
    }

  }  // namespace konieczny_helpers

  //! Provides a call operator returning a hash value for a pair of size_t.
  //!
  //! This struct provides a call operator for obtaining a hash value for the
  //! pair.
  // TODO(now) Replace this with specalization of Hash for pairs
  struct PairHash {
    //! Hashes a pair of size_t.
    size_t operator()(std::pair<size_t, size_t> x) const {
      return std::get<0>(x) + std::get<1>(x) + 0x9e3779b97f4a7c16;
    }
  };

  template <typename TElementType>
  TElementType group_inverse(TElementType id, TElementType bm) {
    TElementType tmp = bm;
    TElementType y;
    do {
      y   = tmp;
      tmp = bm * y;
    } while (tmp != id);
    return y;
  }

  template <typename TElementType = BMat8,
            typename LambdaType      = BMat8,
            typename RhoType         = BMat8>
  class Konieczny {
    using lambda_action_type = ImageRightAction<TElementType, LambdaType>;
    using rho_action_type    = ImageLeftAction<TElementType, RhoType>;
    using lambda_orb_type
        = RightAction<TElementType, LambdaType, lambda_action_type>;
    using rho_orb_type = LeftAction<TElementType, RhoType, rho_action_type>;

   public:
    explicit Konieczny(std::vector<TElementType> const& gens)
        : _rho_orb(),
          _D_classes(),
          _D_rels(),
          _dim(1),
          _gens(gens),
          _group_indices(),
          _group_indices_alt(),
          _unit_in_gens(false),
          _regular_D_classes(),
          _lambda_orb() {
      compute_D_classes();
    }

    ~Konieczny();

    //! Finds a group index of a H class in the R class of \p bm
    size_t find_group_index(TElementType bm) {
      RhoType rv          = Rho<TElementType>()(bm);
      size_t  pos         = _lambda_orb.position(Lambda<TElementType>()(bm));
      size_t  lval_scc_id = _lambda_orb.digraph().scc_id(pos);
      std::pair<size_t, size_t> key = std::make_pair(
          ToInt<RhoType>()(rv), _lambda_orb.digraph().scc_id(pos));

      if (_group_indices.find(key) != _group_indices.end()) {
        return _group_indices.at(key);
      } else {
        for (auto it = _lambda_orb.digraph().cbegin_scc(lval_scc_id);
             it < _lambda_orb.digraph().cend_scc(lval_scc_id);
             it++) {
          if (konieczny_helpers::is_group_index(rv, _lambda_orb.at(*it))) {
            _group_indices.emplace(key, *it);
            return *it;
          }
        }
      }
      _group_indices.emplace(key, UNDEFINED);
      return UNDEFINED;
    }

    bool is_regular_element(TElementType bm) {
      if (find_group_index(bm) != UNDEFINED) {
        return true;
      }
      return false;
    }

    // TODO: it must be possible to do better than this
    TElementType idem_in_H_class(TElementType bm) {
      TElementType tmp = bm;
      while (tmp * tmp != tmp) {
        tmp = tmp * bm;
      }
      return tmp;
    }

    //! Finds an idempotent in the D class of \c bm, if \c bm is regular
    TElementType find_idem(TElementType bm) {
      if (bm * bm == bm) {
        return bm;
      }
      if (!is_regular_element(bm)) {
        // TODO: exception/assertion
        return BMat8(static_cast<size_t>(UNDEFINED));
      }
      size_t          i   = find_group_index(bm);
      size_t          pos = _lambda_orb.position(Lambda<TElementType>()(bm));
      TElementType x   = bm * _lambda_orb.multiplier_to_scc_root(pos)
                          * _lambda_orb.multiplier_from_scc_root(i);
      // BMat8(UNDEFINED) happens to be idempotent...
      return idem_in_H_class(x);
    }

    class BaseDClass;
    class RegularDClass;
    class NonRegularDClass;

    std::vector<RegularDClass*> regular_D_classes() {
      return _regular_D_classes;
    }

    std::vector<BaseDClass*> D_classes() {
      return _D_classes;
    }

    size_t size() const;

   private:
    void add_D_class(
        Konieczny<TElementType, LambdaType, RhoType>::RegularDClass* D);
    void add_D_class(
        Konieczny<TElementType, LambdaType, RhoType>::NonRegularDClass* D);

    //! Finds the minimum dimension \c dim such that all generators have
    //! dimension less than or equal to \c dim, and sets \c _dim.
    void compute_min_possible_dim() {
      _dim = 1;
      for (BMat8 x : _gens) {
        size_t d = bmat8_helpers::min_possible_dim(x);
        if (d > _dim) {
          _dim = d;
        }
      }
    }

    //! Adds the identity TElementType (of degree max of the degrees of the
    //! generators) if there is no permutation already in the generators, and
    //! sets the value of \c _unit_in_gens.
    void conditional_add_identity() {
      // TODO: this isn't quite right - could be 0 generators etc.
      compute_min_possible_dim();
      for (BMat8 x : _gens) {
        if (x * x.transpose() == bmat8_helpers::one<BMat8>(_dim)) {
          _unit_in_gens = true;
        }
      }
      if (!_unit_in_gens) {
        _gens.push_back(bmat8_helpers::one<BMat8>(_dim));
      }
    }

    void compute_orbs() {
      _lambda_orb.add_seed(bmat8_helpers::one<BMat8>(_dim));
      _rho_orb.add_seed(bmat8_helpers::one<BMat8>(_dim));
      for (TElementType g : _gens) {
        _lambda_orb.add_generator(g);
        _rho_orb.add_generator(g);
      }
      _lambda_orb.run();
      _rho_orb.run();
    }

    void compute_D_classes();

    rho_orb_type             _rho_orb;
    std::vector<BaseDClass*> _D_classes;
    // contains in _D_rels[i] the indices of the D classes which lie above
    // _D_classes[i]
    std::vector<std::vector<size_t>> _D_rels;
    size_t                           _dim;
    std::vector<TElementType>     _gens;
    std::unordered_map<std::pair<size_t, size_t>, size_t, PairHash>
        _group_indices;
    std::unordered_map<std::pair<size_t, size_t>, size_t, PairHash>
                                _group_indices_alt;
    bool                        _unit_in_gens;
    std::vector<RegularDClass*> _regular_D_classes;
    lambda_orb_type             _lambda_orb;
  };

  template <typename TElementType, typename LambdaType, typename RhoType>
  class Konieczny<TElementType, LambdaType, RhoType>::BaseDClass {
    friend class Konieczny<TElementType, LambdaType, RhoType>;

   public:
    BaseDClass(Konieczny<TElementType, LambdaType, RhoType>* parent,
               TElementType                                  rep)
        : _rank(KonRank<TElementType>()(rep)),
          _computed(false),
          _H_class(),
          _left_mults(),
          _left_mults_inv(),
          _left_reps(),
          _parent(parent),
          _rep(rep),
          _right_mults(),
          _right_mults_inv(),
          _right_reps() {}

    virtual ~BaseDClass() {}

    TElementType rep() const {
      return _rep;
    }

    typename std::vector<TElementType>::const_iterator cbegin_left_reps() {
      init();
      return _left_reps.cbegin();
    }

    typename std::vector<TElementType>::const_iterator cend_left_reps() {
      init();
      return _left_reps.cend();
    }

    typename std::vector<TElementType>::const_iterator cbegin_right_reps() {
      init();
      return _right_reps.cbegin();
    }

    typename std::vector<TElementType>::const_iterator cend_right_reps() {
      init();
      return _right_reps.cend();
    }

    typename std::vector<TElementType>::const_iterator cbegin_left_mults() {
      init();
      return _left_mults.cbegin();
    }

    typename std::vector<TElementType>::const_iterator cend_left_mults() {
      init();
      return _left_mults.cend();
    }

    typename std::vector<TElementType>::const_iterator cbegin_left_mults_inv() {
      init();
      return _left_mults_inv.cbegin();
    }

    typename std::vector<TElementType>::const_iterator cend_left_mults_inv() {
      init();
      return _left_mults_inv.cend();
    }

    typename std::vector<TElementType>::const_iterator cbegin_right_mults() {
      init();
      return _right_mults.cbegin();
    }

    typename std::vector<TElementType>::const_iterator cend_right_mults() {
      init();
      return _right_mults.cend();
    }

    typename std::vector<TElementType>::const_iterator cbegin_right_mults_inv() {
      init();
      return _right_mults_inv.cbegin();
    }

    typename std::vector<TElementType>::const_iterator cend_right_mults_inv() {
      init();
      return _right_mults_inv.cend();
    }

    typename std::vector<TElementType>::const_iterator cbegin_H_class() {
      init();
      return _H_class.cbegin();
    }

    typename std::vector<TElementType>::const_iterator cend_H_class() {
      init();
      return _H_class.cend();
    }

    virtual bool contains(TElementType bm) = 0;

    bool contains(TElementType bm, size_t rank) {
      return (rank == _rank && contains(bm));
    }

    virtual size_t size() {
      init();
      return _H_class.size() * _left_reps.size() * _right_reps.size();
    }

    std::vector<TElementType> covering_reps() {
      init();
      std::vector<TElementType> out;
      // TODO: how to decide which side to calculate? One is often faster
      if (_parent->_lambda_orb.size() < _parent->_rho_orb.size()) {
        for (TElementType w : _left_reps) {
          for (TElementType g : _parent->_gens) {
            TElementType x = w * g;
            if (!contains(x)) {
              out.push_back(x);
            }
          }
        }
      } else {
        for (TElementType z : _right_reps) {
          for (TElementType g : _parent->_gens) {
            TElementType x = g * z;
            if (!contains(x)) {
              out.push_back(x);
            }
          }
        }
      }
      std::sort(out.begin(), out.end());
      auto it = std::unique(out.begin(), out.end());
      out.erase(it, out.end());
      return out;
    }

   protected:
    virtual void init() = 0;

    size_t                                           _rank;
    bool                                             _computed;
    std::vector<TElementType>                     _H_class;
    std::vector<TElementType>                     _left_mults;
    std::vector<TElementType>                     _left_mults_inv;
    std::vector<TElementType>                     _left_reps;
    Konieczny<TElementType, LambdaType, RhoType>* _parent;
    TElementType                                  _rep;
    std::vector<TElementType>                     _right_mults;
    std::vector<TElementType>                     _right_mults_inv;
    std::vector<TElementType>                     _right_reps;
  };

  template <typename TElementType, typename LambdaType, typename RhoType>
  class Konieczny<TElementType, LambdaType, RhoType>::RegularDClass
      : public Konieczny<TElementType, LambdaType, RhoType>::BaseDClass<TElementType, LambdaType, RhoType> {
   public:
    RegularDClass(Konieczny<TElementType, LambdaType, RhoType>* parent,
                  TElementType                                  idem_rep)
        : Konieczny<TElementType, LambdaType, RhoType>::BaseDClass(parent,
                                                                      idem_rep),
          _rho_val_positions(),
          _H_gens(),
          _left_idem_reps(),
          _left_indices(),
          _right_idem_reps(),
          _right_indices(),
          _lambda_val_positions(),
          _stab_chain() {
      if (idem_rep * idem_rep != idem_rep) {
        LIBSEMIGROUPS_EXCEPTION(
            "RegularDClass: the representative given should be idempotent");
      }
    }

    std::vector<size_t>::const_iterator cbegin_left_indices() {
      init();
      return _left_indices.cbegin();
    }

    std::vector<size_t>::const_iterator cend_left_indices() {
      init();
      return _left_indices.cend();
    }

    std::vector<size_t>::const_iterator cbegin_right_indices() {
      init();
      return _right_indices.cbegin();
    }

    std::vector<size_t>::const_iterator cend_right_indices() {
      init();
      return _right_indices.cend();
    }

    typename std::vector<TElementType>::const_iterator cbegin_left_idem_reps() {
      init();
      return _left_idem_reps.cbegin();
    }

    typename std::vector<TElementType>::const_iterator cend_left_idem_reps() {
      init();
      return _left_idem_reps.cend();
    }

    typename std::vector<TElementType>::const_iterator cbegin_right_idem_reps() {
      init();
      return _right_idem_reps.cbegin();
    }

    typename std::vector<TElementType>::const_iterator cend_right_idem_reps() {
      init();
      return _right_idem_reps.cend();
    }

    /*
    SchreierSims<8, uint8_t, Permutation<uint8_t>> stab_chain() {
      init();
      return _stab_chain;
    }
    */

    // TODO: this is the wrong function! contains shouldn't assume argument is
    //        in semigroup!
    //! Tests whether an element \p bm of the semigroup is in this D class
    //!
    //! Watch out! The element \bm must be known to be in the semigroup for this
    //! to be valid!
    bool contains(TElementType bm) override {
      init();
      std::pair<size_t, size_t> x = index_positions(bm);
      return x.first != UNDEFINED;
    }

    // Returns the indices of the L and R classes of \c this that \p bm is in,
    // unless bm is not in \c this, in which case returns the pair (UNDEFINED,
    // UNDEFINED)
    std::pair<size_t, size_t> index_positions(TElementType bm) {
      init();
      auto l_it = _lambda_val_positions.find(
          ToInt<LambdaType>()(Lambda<TElementType>()(bm)));
      if (l_it != _lambda_val_positions.end()) {
        auto r_it = _rho_val_positions.find(
            ToInt<RhoType>()(Rho<TElementType>()(bm)));
        if (r_it != _rho_val_positions.end()) {
          return std::make_pair((*l_it).second, (*r_it).second);
        }
      }
      return std::make_pair(UNDEFINED, UNDEFINED);
    }

    size_t nr_idempotents() {
      init();
      size_t count = 0;
      for (auto it = cbegin_left_indices(); it < cend_left_indices(); ++it) {
        for (auto it2 = cbegin_right_indices(); it2 < cend_right_indices();
             ++it2) {
          if (konieczny_helpers::is_group_index(_parent->_rho_orb.at(*it2),
                                                _parent->_lambda_orb.at(*it))) {
            count++;
          }
        }
      }
      LIBSEMIGROUPS_ASSERT(count > 0);
      return count;
    }

   private:
    // this is annoyingly a bit more complicated than the right indices
    // because the find_group_index method fixes the rval and loops
    // through the scc of the lval
    void compute_left_indices() {
      if (_left_indices.size() > 0) {
        return;
      }
      size_t lval_pos
          = _parent->_lambda_orb.position(Lambda<TElementType>()(_rep));
      size_t rval_pos
          = _parent->_rho_orb.position(Rho<TElementType>()(_rep));
      size_t lval_scc_id = _parent->_lambda_orb.digraph().scc_id(lval_pos);
      size_t rval_scc_id = _parent->_rho_orb.digraph().scc_id(rval_pos);

      std::pair<size_t, size_t> key = std::make_pair(rval_scc_id, 0);
      for (auto it = _parent->_lambda_orb.digraph().cbegin_scc(lval_scc_id);
           it < _parent->_lambda_orb.digraph().cend_scc(lval_scc_id);
           it++) {
        std::get<1>(key) = *it;
        if (_parent->_group_indices_alt.find(key)
            == _parent->_group_indices_alt.end()) {
          bool found = false;
          for (auto it2 = _parent->_rho_orb.digraph().cbegin_scc(rval_scc_id);
               !found
               && it2 < _parent->_rho_orb.digraph().cend_scc(rval_scc_id);
               it2++) {
            if (konieczny_helpers::is_group_index(
                    _parent->_rho_orb.at(*it2), _parent->_lambda_orb.at(*it))) {
              _parent->_group_indices_alt.emplace(key, *it2);
              found = true;
            }
          }
          if (!found) {
            _parent->_group_indices_alt.emplace(key, UNDEFINED);
          }
        }
        if (_parent->_group_indices_alt.at(key) != UNDEFINED) {
          _lambda_val_positions.emplace(
              ToInt<LambdaType>()(_parent->_lambda_orb.at(*it)),
              _left_indices.size());
          _left_indices.push_back(*it);
        }
      }
#ifdef LIBSEMIGROUPS_DEBUG
      for (size_t i : _left_indices) {
        LIBSEMIGROUPS_ASSERT(i < _parent->_lambda_orb.size());
      }
#endif
    }

    void compute_right_indices() {
      if (_right_indices.size() > 0) {
        return;
      }
      size_t rval_pos
          = _parent->_rho_orb.position(Rho<TElementType>()(_rep));
      size_t rval_scc_id = _parent->_rho_orb.digraph().scc_id(rval_pos);
      for (auto it = _parent->_rho_orb.digraph().cbegin_scc(rval_scc_id);
           it < _parent->_rho_orb.digraph().cend_scc(rval_scc_id);
           it++) {
        TElementType x = _parent->_rho_orb.multiplier_from_scc_root(*it)
                            * _parent->_rho_orb.multiplier_to_scc_root(rval_pos)
                            * _rep;
        if (_parent->find_group_index(x) != UNDEFINED) {
          _rho_val_positions.emplace(
              ToInt<RhoType>()(_parent->_rho_orb.at(*it)),
              _right_indices.size());
          _right_indices.push_back(*it);
        }
      }
#ifdef LIBSEMIGROUPS_DEBUG
      for (size_t i : _right_indices) {
        LIBSEMIGROUPS_ASSERT(i < _parent->_rho_orb.size());
      }
#endif
    }

    void compute_mults() {
      if (_left_mults.size() > 0) {
        return;
      }
      TElementType lval     = Lambda<TElementType>()(_rep);
      size_t          lval_pos = _parent->_lambda_orb.position(lval);
      TElementType rval     = Rho<TElementType>()(_rep);
      size_t          rval_pos = _parent->_rho_orb.position(rval);

      for (size_t i = 0; i < _left_indices.size(); ++i) {
        TElementType b
            = _parent->_lambda_orb.multiplier_to_scc_root(lval_pos)
              * _parent->_lambda_orb.multiplier_from_scc_root(_left_indices[i]);
        TElementType c
            = _parent->_lambda_orb.multiplier_to_scc_root(_left_indices[i])
              * _parent->_lambda_orb.multiplier_from_scc_root(lval_pos);

        _left_mults.push_back(b);
        _left_mults_inv.push_back(c);
      }

      for (size_t i = 0; i < _right_indices.size(); ++i) {
        TElementType c
            = _parent->_rho_orb.multiplier_from_scc_root(_right_indices[i])
              * _parent->_rho_orb.multiplier_to_scc_root(rval_pos);
        TElementType d
            = _parent->_rho_orb.multiplier_from_scc_root(rval_pos)
              * _parent->_rho_orb.multiplier_to_scc_root(_right_indices[i]);

        _right_mults.push_back(c);
        _right_mults_inv.push_back(d);
      }
    }

    void compute_reps() {
      compute_mults();

      _left_reps.clear();
      _right_reps.clear();

      for (TElementType b : _left_mults) {
        _left_reps.push_back(_rep * b);
      }

      for (TElementType c : _right_mults) {
        _right_reps.push_back(c * _rep);
      }
    }

    void compute_H_gens() {
      _H_gens.clear();
      TElementType rval     = Rho<TElementType>()(_rep);
      size_t          rval_pos = _parent->_rho_orb.position(rval);
      size_t rval_scc_id       = _parent->_rho_orb.digraph().scc_id(rval_pos);
      std::vector<TElementType> right_invs;

      for (size_t i = 0; i < _left_indices.size(); ++i) {
        TElementType           p = _left_reps[i];
        std::pair<size_t, size_t> key
            = std::make_pair(rval_scc_id, _left_indices[i]);

        size_t k = _parent->_group_indices_alt.at(key);
        size_t j
            = _rho_val_positions.at(ToInt<RhoType>()(_parent->_rho_orb.at(k)));
        TElementType q = _right_reps[j];
        // find the inverse of pq in H_rep
        TElementType y = group_inverse<TElementType>(_rep, p * q);
        right_invs.push_back(q * y);
      }

      for (size_t i = 0; i < _left_indices.size(); ++i) {
        TElementType p = _left_reps[i];
        for (TElementType g : _parent->_gens) {
          TElementType x = p * g;
          TElementType s = Lambda<TElementType>()(x);
          for (size_t j = 0; j < _left_indices.size(); ++j) {
            if (_parent->_lambda_orb.at(_left_indices[j]) == s) {
              _H_gens.push_back(x * right_invs[j]);
              break;
            }
          }
        }
      }
      std::unordered_set<TElementType> set(_H_gens.begin(), _H_gens.end());
      _H_gens.assign(set.begin(), set.end());
    }

    /*
    void compute_stab_chain() {
      BMat8 row_basis = Lambda<BMat8>()(_rep);
      for (BMat8 x : _H_gens) {
        _stab_chain.add_generator(BMat8::perm_action_on_basis(row_basis, x));
      }
    }
    */

    void compute_idem_reps() {
      TElementType lval     = Lambda<TElementType>()(_rep);
      TElementType rval     = Rho<TElementType>()(_rep);
      size_t          lval_pos = _parent->_lambda_orb.position(lval);
      size_t          rval_pos = _parent->_rho_orb.position(rval);
      size_t lval_scc_id = _parent->_lambda_orb.digraph().scc_id(lval_pos);
      size_t rval_scc_id = _parent->_rho_orb.digraph().scc_id(rval_pos);

      // this all relies on the indices having been computed already

      // TODO: use information from the looping through the left indices in
      // the loop through the right indices
      for (size_t i = 0; i < _left_indices.size(); ++i) {
        std::pair<size_t, size_t> key
            = std::make_pair(rval_scc_id, _left_indices[i]);
        size_t k = _parent->_group_indices_alt.at(key);
        size_t j = 0;
        while (_right_indices[j] != k) {
          ++j;
        }
        TElementType x = _right_mults[j] * _rep * _left_mults[i];
        TElementType y = x;
        // TODO: BMat8(UNDEFINED) happens to be idempotent... genericise!
        while (x * x != x) {
          x = x * y;
        }
        _left_idem_reps.push_back(x);
      }

      for (size_t j = 0; j < _right_indices.size(); ++j) {
        // TODO: make comprehensible
        std::pair<size_t, size_t> key = std::make_pair(
            ToInt<RhoType>()(_parent->_rho_orb.at(_right_indices[j])),
            lval_scc_id);
        size_t k = _parent->_group_indices.at(key);
        size_t i = 0;
        while (_left_indices[i] != k) {
          ++i;
        }
        TElementType x = _right_mults[j] * _rep * _left_mults[i];
        TElementType y = x;
        // TODO: BMat8(UNDEFINED) happens to be idempotent... genericise!
        while (x * x != x) {
          x = x * y;
        }
        _right_idem_reps.push_back(x);
      }
    }

    // there should be some way of getting rid of this
    void compute_H_class() {
      _H_class = std::vector<TElementType>(_H_gens.begin(), _H_gens.end());
      std::unordered_set<TElementType> set(_H_class.begin(), _H_class.end());
      for (size_t i = 0; i < _H_class.size(); ++i) {
        for (TElementType g : _H_gens) {
          TElementType y = _H_class[i] * g;
          if (set.find(y) == set.end()) {
            set.insert(y);
            _H_class.push_back(y);
          }
        }
      }
    }

    void init() override {
      if (_computed) {
        return;
      }
      compute_left_indices();
      compute_right_indices();
      compute_mults();
      compute_reps();
      compute_idem_reps();
      compute_H_gens();
      compute_H_class();
      // compute_stab_chain();
      _computed = true;
    }

    std::unordered_map<size_t, size_t>             _rho_val_positions;
    std::vector<TElementType>                   _H_gens;
    std::vector<TElementType>                   _left_idem_reps;
    std::vector<size_t>                            _left_indices;
    std::vector<TElementType>                   _right_idem_reps;
    std::vector<size_t>                            _right_indices;
    std::unordered_map<size_t, size_t>             _lambda_val_positions;
    SchreierSims<8, uint8_t, Permutation<uint8_t>> _stab_chain;
  };

  template <typename TElementType, typename LambdaType, typename RhoType>
  class Konieczny<TElementType, LambdaType, RhoType>::NonRegularDClass
      : public Konieczny<TElementType, LambdaType, RhoType>::BaseDClass {
    friend class Konieczny<TElementType, LambdaType, RhoType>;

   public:
    NonRegularDClass(Konieczny<TElementType, LambdaType, RhoType>* parent,
                     TElementType                                  rep)
        : Konieczny<TElementType, LambdaType, RhoType>::BaseDClass(parent,
                                                                      rep),
          _rho_val_positions(),
          _left_idem_above(),
          _left_idem_class(),
          _H_set(),
          _right_idem_above(),
          _right_idem_class(),
          _lambda_val_positions() {
      if (rep * rep == rep) {
        LIBSEMIGROUPS_EXCEPTION("NonRegularDClass: the representative "
                                "given should not be idempotent");
      }
    }

    bool contains(TElementType bm) override {
      init();
      size_t x = ToInt<LambdaType>()(Lambda<TElementType>()(bm));
      if (_lambda_val_positions[x].size() == 0) {
        return false;
      }
      size_t y = ToInt<RhoType>()(Rho<TElementType>()(bm));
      for (size_t i : _lambda_val_positions[x]) {
        for (size_t j : _rho_val_positions[y]) {
          if (_H_set.find(_right_mults_inv[j] * bm * _left_mults_inv[i])
              != _H_set.end()) {
            return true;
          }
        }
      }
      return false;
    }

   private:
    void init() override {
      if (_computed) {
        return;
      }
      find_idems_above();
      compute_H_class();
      _computed = true;
    }

    void find_idems_above() {
      // assumes that all D classes above this have already been calculated!
      bool left_found  = false;
      bool right_found = false;
      for (auto it = _parent->_regular_D_classes.rbegin();
           (!left_found || !right_found)
           && it != _parent->_regular_D_classes.rend();
           it++) {
        RegularDClass* D = *it;
        if (!left_found) {
          for (auto idem_it = D->cbegin_left_idem_reps();
               idem_it < D->cend_left_idem_reps();
               idem_it++) {
            if (_rep * (*idem_it) == _rep) {
              _left_idem_above = *idem_it;
              _left_idem_class = D;
              left_found       = true;
              break;
            }
          }
        }

        if (!right_found) {
          for (auto idem_it = D->cbegin_right_idem_reps();
               idem_it < D->cend_right_idem_reps();
               idem_it++) {
            if ((*idem_it) * _rep == _rep) {
              _right_idem_above = (*idem_it);
              _right_idem_class = D;
              right_found       = true;
              break;
            }
          }
        }
      }

#ifdef LIBSEMIGROUPS_DEBUG
      LIBSEMIGROUPS_ASSERT(_left_idem_class->contains(_left_idem_above));
      LIBSEMIGROUPS_ASSERT(_right_idem_class->contains(_right_idem_above));
      LIBSEMIGROUPS_ASSERT(left_found && right_found);
      LIBSEMIGROUPS_ASSERT(_rep * _left_idem_above == _rep);
      LIBSEMIGROUPS_ASSERT(_right_idem_above * _rep == _rep);
#endif
    }

    // TODO: this computes more than just the H class, and should be split
    void compute_H_class() {
      _H_class = std::vector<TElementType>();
      std::pair<size_t, size_t> left_idem_indices
          = _left_idem_class->index_positions(_left_idem_above);
      TElementType left_idem_left_mult
          = _left_idem_class
                ->cbegin_left_mults()[std::get<0>(left_idem_indices)];
      TElementType left_idem_right_mult
          = _left_idem_class
                ->cbegin_right_mults()[std::get<1>(left_idem_indices)];

      std::pair<size_t, size_t> right_idem_indices
          = _right_idem_class->index_positions(_right_idem_above);
      TElementType right_idem_left_mult
          = _right_idem_class
                ->cbegin_left_mults()[std::get<0>(right_idem_indices)];
      TElementType right_idem_right_mult
          = _right_idem_class
                ->cbegin_right_mults()[std::get<1>(right_idem_indices)];

      std::vector<TElementType> _left_idem_H_class;
      std::vector<TElementType> _right_idem_H_class;

      for (auto it = _left_idem_class->cbegin_H_class();
           it < _left_idem_class->cend_H_class();
           it++) {
        _left_idem_H_class.push_back(left_idem_right_mult * (*it)
                                     * left_idem_left_mult);
      }

      for (auto it = _right_idem_class->cbegin_H_class();
           it < _right_idem_class->cend_H_class();
           it++) {
        _right_idem_H_class.push_back(right_idem_right_mult * (*it)
                                      * right_idem_left_mult);
      }

      std::vector<TElementType> left_idem_left_reps;
      std::vector<TElementType> right_idem_right_reps;

      for (auto it = _left_idem_class->cbegin_left_mults();
           it < _left_idem_class->cend_left_mults();
           it++) {
        left_idem_left_reps.push_back(left_idem_right_mult
                                      * _left_idem_class->rep() * (*it));
      }

      for (auto it = _right_idem_class->cbegin_right_mults();
           it < _right_idem_class->cend_right_mults();
           it++) {
        right_idem_right_reps.push_back((*it) * _right_idem_class->rep()
                                        * right_idem_left_mult);
      }

      std::vector<TElementType> Hex;
      std::vector<TElementType> xHf;

      for (TElementType s : _left_idem_H_class) {
        xHf.push_back(_rep * s);
      }

      for (TElementType t : _right_idem_H_class) {
        Hex.push_back(t * _rep);
      }

      std::unordered_set<TElementType> s(Hex.begin(), Hex.end());
      Hex.assign(s.begin(), s.end());

      s = std::unordered_set<TElementType>(xHf.begin(), xHf.end());
      xHf.assign(s.begin(), s.end());

      std::sort(Hex.begin(), Hex.end());
      std::sort(xHf.begin(), xHf.end());

      std::set_intersection(Hex.begin(),
                            Hex.end(),
                            xHf.begin(),
                            xHf.end(),
                            std::back_inserter(_H_class));
      for (TElementType x : _H_class) {
        _H_set.insert(x);
      }

      //////////////////////////////////////////
      // The rest of the function is multipliers and should be a difference
      // function... how to split without copying/storing unnecessary data?

      _left_reps.clear();
      _left_mults.clear();
      _right_reps.clear();
      _right_mults.clear();

      std::unordered_set<std::vector<TElementType>, VecHash<TElementType>>
          Hxhw_set;
      std::unordered_set<std::vector<TElementType>, VecHash<TElementType>>
          zhHx_set;

      for (TElementType h : _left_idem_H_class) {
        for (size_t i = 0; i < left_idem_left_reps.size(); ++i) {
          TElementType w = left_idem_left_reps[i];
          // TODO: enforce uniqueness here?
          std::vector<TElementType> Hxhw;
          for (TElementType s : _H_class) {
            Hxhw.push_back(s * h * w);
          }
          std::sort(Hxhw.begin(), Hxhw.end());
          if (Hxhw_set.find(Hxhw) == Hxhw_set.end()) {
            Hxhw_set.insert(Hxhw);
            TElementType A = _rep * h * w;
            TElementType inv
                = group_inverse<TElementType>(
                      _left_idem_above,
                      w * _left_idem_class->cbegin_left_mults_inv()[i]
                          * left_idem_left_mult)
                  * group_inverse<TElementType>(_left_idem_above, h);

            size_t x  = ToInt<LambdaType>()(Lambda<TElementType>()(A));
            auto   it = _lambda_val_positions.find(x);
            if (it == _lambda_val_positions.end()) {
              _lambda_val_positions.emplace(x, std::vector<size_t>());
            }
            _lambda_val_positions[x].push_back(_left_reps.size());
            _left_reps.push_back(A);
            _left_mults.push_back(h * w);
            _left_mults_inv.push_back(
                _left_idem_class->cbegin_left_mults_inv()[i]
                * left_idem_left_mult * inv);
          }
        }
      }

      for (TElementType h : _right_idem_H_class) {
        for (size_t i = 0; i < right_idem_right_reps.size(); ++i) {
          TElementType              z = right_idem_right_reps[i];
          std::vector<TElementType> zhHx;
          for (TElementType s : _H_class) {
            zhHx.push_back(z * h * s);
          }
          std::sort(zhHx.begin(), zhHx.end());
          if (zhHx_set.find(zhHx) == zhHx_set.end()) {
            zhHx_set.insert(zhHx);
            TElementType B = z * h * _rep;
            TElementType inv
                = group_inverse<TElementType>(_right_idem_above, h)
                  * group_inverse<TElementType>(
                        _right_idem_above,
                        right_idem_right_mult
                            * _right_idem_class->cbegin_right_mults_inv()[i]
                            * z);

            size_t x  = ToInt<RhoType>()(Rho<TElementType>()(B));
            auto   it = _rho_val_positions.find(x);
            if (it == _rho_val_positions.end()) {
              _rho_val_positions.emplace(x, std::vector<size_t>());
            }
            _rho_val_positions[x].push_back(_right_reps.size());
            _right_reps.push_back(B);
            _right_mults.push_back(z * h);
            _right_mults_inv.push_back(
                inv * right_idem_right_mult
                * _right_idem_class->cbegin_right_mults_inv()[i]);
          }
        }
      }
    }

    std::unordered_map<size_t, std::vector<size_t>> _rho_val_positions;
    TElementType                                 _left_idem_above;
    RegularDClass*                                  _left_idem_class;
    std::unordered_set<TElementType>             _H_set;
    TElementType                                 _right_idem_above;
    RegularDClass*                                  _right_idem_class;
    std::unordered_map<size_t, std::vector<size_t>> _lambda_val_positions;
  };

  template <typename TElementType, typename LambdaType, typename RhoType>
  Konieczny<TElementType, LambdaType, RhoType>::~Konieczny() {
    for (BaseDClass* D : _D_classes) {
      delete D;
    }
  }

  template <typename TElementType, typename LambdaType, typename RhoType>
  void Konieczny<TElementType, LambdaType, RhoType>::add_D_class(
      Konieczny::RegularDClass* D) {
    _regular_D_classes.push_back(D);
    _D_classes.push_back(D);
    _D_rels.push_back(std::vector<size_t>());
  }

  template <typename TElementType, typename LambdaType, typename RhoType>
  void Konieczny<TElementType, LambdaType, RhoType>::add_D_class(
      Konieczny::NonRegularDClass* D) {
    _D_classes.push_back(D);
    _D_rels.push_back(std::vector<size_t>());
  }

  template <typename TElementType, typename LambdaType, typename RhoType>
  size_t Konieczny<TElementType, LambdaType, RhoType>::size() const {
    size_t out = 0;
    auto   it  = _D_classes.begin();
    if (!_unit_in_gens) {
      it++;
    }
    for (; it < _D_classes.end(); it++) {
      out += (*it)->size();
    }
    return out;
  }

  template <typename TElementType, typename LambdaType, typename RhoType>
  void Konieczny<TElementType, LambdaType, RhoType>::compute_D_classes() {
    conditional_add_identity();
    compute_orbs();

    // TODO: genericise
    std::vector<std::vector<std::pair<TElementType, size_t>>> reg_reps(
        257, std::vector<std::pair<TElementType, size_t>>());
    std::vector<std::vector<std::pair<TElementType, size_t>>> non_reg_reps(
        257, std::vector<std::pair<TElementType, size_t>>());
    // TODO: reserve?
    std::set<size_t> ranks;
    size_t           max_rank = 0;
    ranks.insert(0);


    // TODO: genericise
    RegularDClass* top
        = new RegularDClass(this, bmat8_helpers::one<BMat8>(_dim));
    add_D_class(top);
    for (TElementType x : top->covering_reps()) {
      size_t rank = KonRank<TElementType>()(x);
      ranks.insert(rank);
      if (is_regular_element(x)) {
        reg_reps[rank].push_back(std::make_pair(x, 0));
      } else {
        non_reg_reps[rank].push_back(std::make_pair(x, 0));
      }
    }

    while (*ranks.rbegin() > 0) {
      size_t                                          reps_are_reg = false;
      std::vector<std::pair<TElementType, size_t>> next_reps;

      max_rank = *ranks.rbegin();
      if (!reg_reps[max_rank].empty()) {
        reps_are_reg = true;
        next_reps    = std::move(reg_reps[max_rank]);
        reg_reps[max_rank].clear();
      } else {
        next_reps = std::move(non_reg_reps[max_rank]);
        non_reg_reps[max_rank].clear();
      }

      std::vector<std::pair<TElementType, size_t>> tmp_next;
      for (auto it = next_reps.begin(); it < next_reps.end(); it++) {
        bool contained = false;
        for (size_t i = 0; i < _D_classes.size(); ++i) {
          if (_D_classes[i]->contains(std::get<0>(*it), max_rank)) {
            _D_rels[i].push_back(std::get<1>(*it));
            contained = true;
            break;
          }
        }
        if (!contained) {
          tmp_next.push_back(*it);
        }
      }
      next_reps = std::move(tmp_next);

      while (!next_reps.empty()) {
        BaseDClass*                         D;
        std::tuple<TElementType, size_t> tup;

        if (reps_are_reg) {
          tup = next_reps.back();
          D   = new RegularDClass(this, find_idem(std::get<0>(tup)));
          add_D_class(static_cast<RegularDClass*>(D));
          for (TElementType x : D->covering_reps()) {
            size_t rank = KonRank<TElementType>()(x);
            ranks.insert(rank);
            if (is_regular_element(x)) {
              reg_reps[rank].push_back(
                  std::make_pair(x, _D_classes.size() - 1));
            } else {
              non_reg_reps[rank].push_back(
                  std::make_pair(x, _D_classes.size() - 1));
            }
          }
          next_reps.pop_back();

        } else {
          tup = next_reps.back();
          D   = new NonRegularDClass(this, std::get<0>(tup));
          add_D_class(static_cast<NonRegularDClass*>(D));
          for (TElementType x : D->covering_reps()) {
            size_t rank = KonRank<TElementType>()(x);
            ranks.insert(rank);
            if (is_regular_element(x)) {
              reg_reps[rank].push_back(
                  std::make_pair(x, _D_classes.size() - 1));
            } else {
              non_reg_reps[rank].push_back(
                  std::make_pair(x, _D_classes.size() - 1));
            }
          }
          next_reps.pop_back();
        }

        std::vector<std::pair<TElementType, size_t>> tmp;
        for (std::pair<TElementType, size_t>& x : next_reps) {
          if (D->contains(std::get<0>(x))) {
            _D_rels[_D_classes.size() - 1].push_back(std::get<1>(x));
          } else {
            tmp.push_back(std::move(x));
          }
        }
        next_reps = std::move(tmp);
      }
      LIBSEMIGROUPS_ASSERT(reg_reps[max_rank].empty());
      if (non_reg_reps[max_rank].empty()) {
        ranks.erase(max_rank);
        max_rank = *ranks.rbegin();
      }
    }
  }
}  // namespace libsemigroups
#endif  // LIBSEMIGROUPS_INCLUDE_KONIECZNY_HPP_
