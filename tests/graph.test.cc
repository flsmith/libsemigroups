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

#include <iostream>

#include "catch.hpp"

#include "../src/graph.h"

using namespace libsemigroups;

TEST_CASE("Graph 01: Default constructor with 2 default args",
          "[quick][graph][01]") {
  Graph g = Graph();
  REQUIRE(g.nr_rows() == 0);
  REQUIRE(g.nr_nodes() == 0);
  REQUIRE(g.nr_cols() == 0);
  REQUIRE(g.nr_edges() == 0);
}

TEST_CASE("Graph 02: Default constructor with 1 default arg",
          "[quick][graph][02]") {
  Graph g = Graph(10);
  REQUIRE(g.nr_nodes() == 0);
  REQUIRE(g.nr_cols() == 10);
  REQUIRE(g.nr_edges() == 0);
}

TEST_CASE("Graph 03: Default constructor with 0 default args",
          "[quick][graph][03]") {
  Graph g = Graph(10, 7);
  REQUIRE(g.nr_nodes() == 7);
  REQUIRE(g.nr_cols() == 10);
  REQUIRE(g.nr_edges() == 0);
}

TEST_CASE("Graph 04: add nodes",
          "[quick][graph][04]") {
  Graph g = Graph(10, 7);
  REQUIRE(g.nr_nodes() == 7);
  REQUIRE(g.nr_cols() == 10);
  REQUIRE(g.nr_edges() == 0);
  
  for (size_t i = 1; i < 100; ++i){
    g.add_nodes(i);
    REQUIRE(g.nr_nodes() == 7 + i*(i + 1)/2);
  }

  for (size_t i = 0; i < 100; ++i) {
    for (size_t j = 0; j < 10; ++j) {
      REQUIRE(g.get(i, j) == Graph::UNDEFINED);
    }
  }
}

TEST_CASE("Graph 05: add edges",
          "[quick][graph][05]") {
  Graph g = Graph(1,1);
 
  for (size_t i = 0; i < 17; ++i){
    for(size_t j = 0; j < 30; ++j){
      g.add_edge(i, (7 * i + 23 * j) % 17);
    }
  }
  
  REQUIRE(g.nr_cols() == 30);
  REQUIRE(g.nr_nodes() == 17);

  for (size_t i = 0; i < g.nr_nodes(); ++i){
    for (auto it = g.begin_row(i); it < g.end_row(i); ++it){
      size_t j = it - g.begin_row(i);
      REQUIRE(g.get(i, j) == (7 * i + 23 * j) % 17);
    }   
  }
}


TEST_CASE("Graph 06: add edges to empty graph",
          "[quick][graph][06]") {
  Graph g = Graph();

  for (size_t i = 0; i < 17; ++i){
    for(size_t j = 0; j < 30; ++j){
      g.add_edge(i, (7 * i + 23 * j) % 17);
    }
  } 
  
  REQUIRE(g.nr_cols() == 30);
  REQUIRE(g.nr_nodes() == 17);
  
  for (size_t i = 0; i < g.nr_nodes(); ++i){
    for (auto it = g.begin_row(i); it < g.end_row(i); ++it){
      size_t j = it - g.begin_row(i);
      REQUIRE(g.get(i, j) == (7 * i + 23 * j) % 17);
    }   
  }
}

TEST_CASE("Graph 07: tidy",
          "[quick][graph][07]"){
  Graph g = Graph();

  for (size_t i = 0; i < 100; ++i){
    for(size_t j = 0; j < 100; ++j){
      g.add_edge(i, (73 * i + 85 * j) % 100);
      if (g.get(i, j) % 13 == 0){
        g.set(i, j, Graph::UNDEFINED);
      }
    }
  }

  g.tidy();

  for (size_t i = 0; i < 100; ++i) {
    for (auto it = g.begin_row(i); it < g.end_row(i) - 1; ++it) {
      REQUIRE(*(it + 1) >= *it);
    }
  }
}

TEST_CASE("Graph 08: Strongly connected components",
          "[quick][graph][08]") {
  Graph cycle = Graph();
  
  for (size_t i = 0; i < 10; ++i) {
    cycle.add_edge(i, i + 1);
  }
  cycle.add_edge(10, 0);
  cycle.gabow_scc();
  
  for (size_t i : cycle.scc_ids()){
    std::cout << i << std::endl;
  }
}




