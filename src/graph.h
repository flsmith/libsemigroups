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

#ifndef LIBSEMIGROUPS_SRC_GRAPH_H_
#define LIBSEMIGROUPS_SRC_GRAPH_H_

#include <stack>
#include "recvec.h"

namespace libsemigroups {
  // Represents vertices as rows
  // There is an edge (i, j) in the graph iff j occurs in row i
  //
  // TODO: just make these regular
  // TODO: proper commentary (makedoc)
  // TODO: maybe mutability
  class Graph : public RecVec<size_t> {
    typedef RecVec<size_t, std::allocator<size_t>> base_recvec;

  public:
    // if we have graphs with max of size_t vertices we have bigger problems
    static const size_t UNDEFINED;

    explicit Graph(size_t max_degree = 0, size_t nr_vertices = 0)
        : base_recvec(max_degree, nr_vertices, UNDEFINED), _cc_comps(), _cc_ids(),
          _has_scc(false), _next_edge_pos(nr_vertices) {}

    Graph &operator=(Graph const &graph) = delete;

    void inline set(size_t i, size_t j, size_t k) {
      base_recvec::set(i, j, k);
      _has_scc = false;
    }

    void inline add_nodes(size_t nr) {
      base_recvec::add_rows(nr);
      for (size_t i = 0; i < nr; ++i) {
        _next_edge_pos.push_back(0);
      }
      _has_scc = false;
    }

    size_t inline nr_nodes() { return nr_rows(); }

    void inline add_edge(size_t i, size_t j) {
      resize_set(i, i < _nr_rows ?_next_edge_pos[i] : 0, j);
      ++_next_edge_pos[i];
    }

    size_t inline nr_edges() {
      size_t edges = 0;
      for (size_t i = 0; i < _nr_rows; ++i) {
        count += std::count_if(begin_row(i), end_row(i),
                               [](size_t j) { return j != UNDEFINED; });
      }
      return edges;
    }

    void tidy() {
      // something is bound to go wrong here
      for (size_t i = 0; i < _nr_rows; ++i) {
        std::sort(begin_row(i), end_row(i));
        _next_edge_pos[i] = std::count_if(
            begin_row(i), end_row(i), [](size_t j) { return j != UNDEFINED; });
      }
    }

    // Gabow's Strongly Connected Component algorithm
    // strongly based on the implementation
    // https://algs4.cs.princeton.edu/42digraph/GabowSCC.java.html
    // TODO: make non-recursive
    void dive(size_t i) {
      visited[i] = true;
      preorder[i] = _pre++;
      s1.push(i);
      s2.push(i);
      
      // std::cout << "entered dive with i = " << i << std::endl;

      // assumes we've tidied the graph
      for (size_t j = 0; j < _nr_used_cols; j++) {
        size_t k = get(i, j);
        //std::cout << "j = " << j << ", k = " << k << std::endl;
        //std::cout << k << " visited: " << visited[k] << ", k id: " << _cc_ids[k] << std::endl;
        if (k == UNDEFINED) {
          break;
        }
        if (!visited[k]) {
          dive(k);
        } else if (_cc_ids[k] == UNDEFINED) {
          while (preorder[s2.top()] > preorder[k]) {
            s2.pop();
          }
        }
      }

      if (s2.top() == i) {
        s2.pop();
        size_t m = UNDEFINED;
        do {
          m = s1.top();
          s1.pop();
          _cc_ids[m] = count;
          //std::cout << "popped, i = " << i << ", m = " << m << ", count = " << count << std::endl;
        } while (i != m);
        count++;
      }
    }

    void gabow_scc() {
      _cc_ids = std::vector<size_t>(_nr_rows, UNDEFINED);
      _cc_comps.clear();
      preorder = std::vector<size_t>(_nr_rows, UNDEFINED);
      visited = std::vector<bool>(_nr_rows, false);
      _pre = 0;
      tidy();
      count = 0;

      for (size_t i = 0; i < _nr_rows; ++i) {
        if (!visited[i]) {
          dive(i);
        }
      }
      for (size_t i = 0; i < count; ++i) {
        _cc_comps.push_back(std::vector<size_t>());
      }
      for (size_t i = 0; i < _nr_rows; ++i) {
        _cc_comps[_cc_ids[i]].push_back(i);
      }
    }

    //for debug only. should be removed
    std::vector<size_t> scc_ids(){
      return _cc_ids;
    }

  private:
    // resizes the graph to have the necessary number of rows and columns
    void resize_set(size_t i, size_t j, size_t k) {
      // row indexing starts at 0, nr_rows starts at 1
      // TODO: make this more obvious
      if (i >= (static_cast<int>(_nr_rows - 1) < 0 ? 0 : _nr_rows)) {
        add_nodes(i - _nr_rows + 1);
      }

      if (j >= (static_cast<int>(_nr_used_cols - 1) < 0 ? 0 : _nr_used_cols)) {
        base_recvec::add_cols(j - _nr_used_cols + 1);
      }

      if (k >= (static_cast<int>(_nr_rows - 1) < 0 ? 0 : _nr_rows)) {
        add_nodes(k - _nr_rows + 1);
      }

      base_recvec::set(i, j, k);
      _has_scc = false;
    }

    // TODO: prune data
    std::vector<std::vector<size_t>> _cc_comps;
    std::vector<size_t> _cc_ids;

    // TODO: prune these when gabow made non-recursive
    size_t count;
    bool _has_scc;
    std::vector<size_t> _next_edge_pos;
    std::vector<size_t> preorder;
    std::stack<size_t> s1;
    std::stack<size_t> s2;
    std::vector<bool> visited;
    size_t _pre;
  };

  const size_t Graph::UNDEFINED = std::numeric_limits<size_t>::max();
}  // namespace libsemigroups
#endif  // LIBSEMIGROUPS_SRC_GRAPH_H_
