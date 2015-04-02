#ifndef RESAMPLE_ALGOS_HPP
#define RESAMPLE_ALGOS_HPP

#include <algorithm>
#include <iostream>

#include "grid.hpp"

template<class T>
class NearestNeighbor {
public:
  T getValue(const Grid<T>& src, double x, double y) const {
    return *src.at(
        static_cast<index_t>(round(y)),
        static_cast<index_t>(round(x))
    );
  }
};

// Given valueA at posA, and valueB at posB, this function interpolates the
// value at pos using a linear weighting.
static inline double linear_interp(double pos,
  double posA, double posB,
  double valueA, double valueB) {

  double dist = posB - posA;
  if (dist == 0)
    return valueA;
  return valueB * (pos - posA)/dist + valueA * (posB - pos)/dist;
}

template<class T>
class Bilinear {
public:
  double getValue(const Grid<T>& src, double x, double y) const {
    index_t x1 = std::floor(x), x2 = std::ceil(x);
    index_t y1 = std::floor(y), y2 = std::ceil(y);

    // Get the values at the four pixels surrounding x/y.
    double nw = *src.at(y1, x1);
    double ne = *src.at(y1, x2);
    double sw = *src.at(y2, x1);
    double se = *src.at(y2, x2);

    // Combine the two northern points using linear interpolation.
    double n = linear_interp(x, x1, x2, nw, ne);
    // Combine the two southern points using linear interpolation.
    double s = linear_interp(x, x1, x2, sw, se);
    // Combine the calculated north and south values.
    return linear_interp(y, y1, y2, n, s);
  }
};

#endif

