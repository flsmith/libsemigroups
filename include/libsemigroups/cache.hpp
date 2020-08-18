//
// libsemigroups - C++ library for semigroups and monoids
// Copyright (C) 2020
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

// This file contains an implementation of caches, which provide an easy
// way to manage temporary elements while avoiding unnecessary memory
// allocation.

#ifndef LIBSEMIGROUPS_CACHE_HPP_
#define LIBSEMIGROUPS_CACHE_HPP_

#include <list>           // for list
#include <stack>          // for stack
#include <type_traits>    // for is_pointer
#include <unordered_map>  // for unordered_map

#include "libsemigroups/libsemigroups-exception.hpp"  // for LIBSEMIGROUPS_EXCEPTION

namespace libsemigroups {
  namespace detail {

    // Declarations
    template <typename T, typename = void>
    class Cache;

    template <typename T, typename = void>
    class CacheGuard;

    // Helpers
    template <typename T>
    using is_pointer_t =
        typename std::enable_if<std::is_pointer<T>::value>::type;

    template <typename T>
    using is_non_pointer_t =
        typename std::enable_if<!std::is_pointer<T>::value>::type;

    // Cache for non-pointer types, these are intended to be used with the
    // T = BruidhinnTraits::internal_value_type, and so the non-pointer objects
    // should be small and easy to copy. There are only versions for pointer
    // and non-pointer types as BruidhinnTraits::internal_value_type is always
    // one of these two. It's your responsibility to ensure that the
    // non-pointer type is actually small and easy to copy.
    template <typename T>
    class Cache<T, is_non_pointer_t<T>> final {
      static_assert(std::is_default_constructible<T>::value,
                    "Cache<T> requires T to be default-constructible");

     public:
      Cache()  = default;
      ~Cache() = default;

      // Deleted other constructors to avoid unintentional copying
      Cache(Cache const&) = delete;
      Cache(Cache&&)      = delete;
      Cache& operator=(Cache const&) = delete;
      Cache& operator=(Cache&&) = delete;

      T acquire() {
        return T();
      }

      void release(T&) {}

      void push(T const&, size_t = 1) {}
    };

    template <typename T>
    class Cache<T, is_pointer_t<T>> final {
     public:
      // Not noexcept because default constructors of, say, std::list isn't
      Cache() = default;
      ~Cache() {
        while (!_acquirable.empty()) {
          delete _acquirable.top();
          _acquirable.pop();
        }
        while (!_acquired.empty()) {
          delete _acquired.back();
          _acquired.pop_back();
        }
      }

      // Deleted other constructors to avoid unintentional copying
      Cache(Cache const&) = delete;
      Cache(Cache&&)      = delete;
      Cache& operator=(Cache const&) = delete;
      Cache& operator=(Cache&&) = delete;

      // Not noexcept because it can throw
      T acquire() {
        if (_acquirable.empty()) {
          LIBSEMIGROUPS_EXCEPTION(
              "attempted to acquire an object, but none are acquirable");
        }
        T ptr = _acquirable.top();
        _acquirable.pop();
        _acquired.push_back(ptr);
        _map.emplace(ptr, --_acquired.end());
        return ptr;
      }

      // Not noexcept because it can throw
      void release(T ptr) {
        auto it = _map.find(ptr);
        if (it == _map.end()) {
          LIBSEMIGROUPS_EXCEPTION("attempted to release an object which is "
                                  "not owned by this cache");
        }
        _acquired.erase(it->second);
        _map.erase(it);
        _acquirable.push(ptr);
      }

      // Not noexcept
      void push(T x, size_t number = 1) {
        for (size_t i = 0; i < number; ++i) {
          _acquirable.push(new (typename std::remove_pointer<T>::type)(*x));
        }
      }

     private:
      std::stack<T>                                          _acquirable;
      std::list<T>                                           _acquired;
      std::unordered_map<T, typename std::list<T>::iterator> _map;
    };

    // A cache guard acquires an element from the cache on construction and
    // releases it on destruction. This version doesn't do anything, it's based
    // on the assumption that the type T is small and easy to copy.
    template <typename T>
    class CacheGuard<T, is_non_pointer_t<T>> final {
      static_assert(std::is_default_constructible<T>::value,
                    "CacheGuard<T> requires T to be default-constructible");

     public:
      explicit CacheGuard(Cache<T> const&) {}
      ~CacheGuard() = default;

      // Deleted other constructors to avoid unintentional copying
      CacheGuard()                  = delete;
      CacheGuard(CacheGuard const&) = delete;
      CacheGuard(CacheGuard&&)      = delete;
      CacheGuard& operator=(CacheGuard const&) = delete;
      CacheGuard& operator=(CacheGuard&&) = delete;

      // Get the element acquired from the cache
      T get() const noexcept {
        return T();
      }
    };

    // A cache guard acquires an element from the cache on construction and
    // releases it on destruction.
    template <typename T>
    class CacheGuard<T, is_pointer_t<T>> final {
     public:
      explicit CacheGuard(Cache<T>& cache)
          : _cache(cache), _tmp(cache.acquire()) {}

      ~CacheGuard() {
        _cache.release(_tmp);
      }

      // Deleted other constructors to avoid unintentional copying
      CacheGuard()                  = delete;
      CacheGuard(CacheGuard const&) = delete;
      CacheGuard(CacheGuard&&)      = delete;
      CacheGuard& operator=(CacheGuard const&) = delete;
      CacheGuard& operator=(CacheGuard&&) = delete;

      // Get the element acquired from the cache
      T get() const noexcept {
        return _tmp;
      }

     private:
      Cache<T>& _cache;
      T         _tmp;
    };

  }  // namespace detail
}  // namespace libsemigroups

#endif  // LIBSEMIGROUPS_CACHE_HPP_
