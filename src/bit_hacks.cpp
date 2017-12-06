// g++ -O3 -std=c++11 -Wall -Wextra -pedantic -o bit_hacks bit_hacks.cpp

#include <array>
#include <algorithm>
#include <iostream>
#include <climits>
#include <random>
#include <string>
#include <vector>

#include "timer.h"

#define intersect(A, B) ~(~A & B) & B

static uint64_t val = 255;

static std::vector<uint64_t> mask = {val,       val << 8,  val << 16,
                                     val << 24, val << 32, val << 40,
                                     val << 48, val << 56};

static std::array<uint64_t, 8> for_sorting{{0, 0, 0, 0, 0, 0, 0, 0}};

// cyclically shifts bits to left by 8
// https://stackoverflow.com/a/776523
static inline uint64_t cyclic_shift(uint64_t n) {
  const unsigned int mask =
      (CHAR_BIT * sizeof(n) - 1); // assumes width is a power of 2.

  // assert ( (c<=mask) &&"rotate by type width or more");
  unsigned int c = 8;
  c &= mask;
  return (n << c) | (n >> ((-c) & mask));
}

// Given 8-by-8 matrices A,B, foo returns a matrix
// where each row is the corresponding row of B if
// that row of B is a subset of the corresponding row of A,
// and 0 otherwise.
// http://graphics.stanford.edu/~seander/bithacks.html#ConditionalSetOrClearBitsWithoutBranching
/*
static inline uint64_t foo(uint64_t A, uint64_t B) {
  uint64_t w = intersect(A, B);
  for (size_t i = 0; i < 8; ++i) {
    w ^= (-((w & mask[i]) == (B & mask[i])) ^ w) & (B & mask[i]);
  }
  return w;
}
*/

// Does the same thing as foo, but is faster
static inline uint64_t bar(uint64_t A, uint64_t B) {
  uint64_t w = intersect(A, B);
  for (size_t i = 0; i < 8; ++i) {
    if ((w & mask[i]) == (B & mask[i])){
      w |= (B & mask[i]);
    } else{
      w &= ~(B & mask[i]);
    }
  }
  return w;
}

void sort_rows_bmat(uint64_t& bm) {
  for (size_t i = 0; i < 8; ++i) {
    for_sorting[i] = (bm << 8 * i) & mask[7];
  }
  std::sort(for_sorting.begin(), for_sorting.end());
  bm = 0;
  for (size_t i = 0; i < 8; ++i) {
    bm |= for_sorting[i];
    bm = (bm >> 8);
  }
}

// https://chessprogramming.wikispaces.com/Flipping+Mirroring+and+Rotating#FlipabouttheAntidiagonal


class BooleanMat {
 public:
  BooleanMat(std::initializer_list<std::initializer_list<int>> mat) {
    // mat should be square, and not of length greater than 8
    _data = 0;
    uint64_t pow  = 1;
    pow           = pow << 63;
    for (auto row : mat) {
      for (auto entry : row) {
        if (entry) {
          _data ^= pow;
        }
        pow = pow >> 1;
      }
      pow = pow >> (8 - mat.size());
    }
  }

  BooleanMat(uint64_t mat) {
    _data = mat;
  }

  std::string to_string() {
    std::string out;
    uint64_t bm = _data;
    uint64_t    pow = 1;
    pow             = pow << 63;
    for (size_t i = 0; i < 8; ++i) {
      for (size_t j = 0; j < 8; ++j) {
        if (pow & bm) {
          out += "1";
        } else {
          out += "0";
        }
        bm = bm << 1;
      }
      out += "\n";
    }
    return out;
  }

  BooleanMat row_space_basis() {
    uint64_t out = 0;
    uint64_t cm  = _data;
    for (size_t i = 0; i < 7; ++i) {
      cm = cyclic_shift(cm);
      out |= bar(_data, cm);
    }
    for (size_t i = 0; i < 8; ++i) {
      // TODO remove if-statement as per foo
      if ((out & mask[i]) == (_data & mask[i])) {
        out &= ~mask[i];
      } else {
        out |= (_data & mask[i]);
      }
    }
    sort_rows_bmat(out);
    return BooleanMat(out);
  }

  BooleanMat transpose() {
    uint64_t x = _data;
    uint64_t y;
    uint64_t m1 = 47851476196393130;
    uint64_t m2 = 225176545447116;
    uint64_t m3 = 4042322160;

    y = (x ^ (x >> 7)) & m1;
    x = x ^ y ^ (y << 7);
    y = (x ^ (x >> 14)) & m2;
    x = x ^ y ^ (y << 14);
    y = (x ^ (x >> 28)) & m3;
    x = x ^ y ^ (y << 28);

    return x;
  }

  uint64_t get_data() {
    return _data;
  } 

 private:
  uint64_t _data;
};

int main() {

  libsemigroups::Timer t;

  uint64_t a0 = 14;
  uint64_t a1 = 6;
  uint64_t a2 = 8;
  uint64_t a3 = 5;
/*
  std::cout << " expect " << a1 << " got " << bar(a0, a1) <<"\n";
  std::cout << " expect " << a2 << " got " << bar(a0, a2) << "\n";
  std::cout << " expect 0 got " << bar(a0, a3) << "\n";

  std::cout << " expect 0 got " << bar(a1, a0) << "\n";
  std::cout << " expect 0 got " << bar(a1, a2) << "\n";
  std::cout << " expect 0 got " << bar(a1, a3) << "\n";

  std::cout << " expect 0 got " << bar(a2, a0) << "\n";
  std::cout << " expect 0 got " << bar(a2, a1) << "\n";
  std::cout << " expect 0 got " << bar(a2, a3) << "\n";

  std::cout << " expect 0 got " << bar(a3, a0) << "\n";
  std::cout << " expect 0 got " << bar(a3, a1) << "\n";
  std::cout << " expect 0 got " << bar(a3, a2) << "\n";

  std::cout << " expect " << (size_t)std::pow(2, 8) << " got "
            << cyclic_shift(1) << "\n";
  std::cout << " expect " << (size_t)std::pow(2, 16) << " got "
            << cyclic_shift(256) << "\n";
  std::cout << " expect " << (size_t)std::pow(2, 32) << " got "
            << cyclic_shift(std::pow(2, 24)) << "\n";
  std::cout << " expect 1 got " << cyclic_shift(std::pow(2, 56)) << "\n";

  BooleanMat bm = BooleanMat({{1, 1, 1, 0}, {0, 1, 1, 0}, {1, 0, 0, 0}, {0, 1, 0, 1}});
  std::cout << bm.to_string() << std::endl;
  std::cout << bm.row_space_basis().to_string() << std::endl;
  bm = BooleanMat({{0, 1, 1, 0}, {1, 1, 1, 0}, {0, 1, 0, 1}, {1, 0, 0, 0}});
  std::cout << bm.to_string() << std::endl;
  std::cout << bm.row_space_basis().to_string() << std::endl;
*/
  
  BooleanMat bm = BooleanMat({{1, 1, 1, 0}, {0, 1, 1, 0}, {1, 0, 0, 0}, {0, 1, 0, 1}});
  BooleanMat bmt = BooleanMat({{1, 0, 1, 0}, {1, 1, 0, 1}, {1, 1, 0, 0}, {0, 0, 0, 1}});
  std::cout << " expect \n" << bmt.to_string() << " got \n" << bm.transpose().to_string();
  



  /*
  t.start();
  size_t count = 0;
  for (uint64_t data = 0; count < 10000000; data += 1213122121787439, count++) {
    BooleanMat bm = BooleanMat(data);
    bm.row_space_basis();
  }

  t.stop();
  std::cout << t.elapsed() << std::endl;
  std::cout << count << std::endl;
  */
  
  t.start();
  size_t count = 0;
  for (uint64_t data = 0; count < 1000000000; data += 1213122121787439, count++) {
    BooleanMat bm = BooleanMat(data);
    bm.transpose();
  }

  t.stop();
  std::cout << t.string() << std::endl;
  std::cout << count << std::endl;

  return 0;
}
