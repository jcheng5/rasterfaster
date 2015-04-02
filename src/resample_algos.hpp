#ifndef RESAMPLE_ALGOS_HPP
#define RESAMPLE_ALGOS_HPP

#include <algorithm>
#include <iostream>

#include <boost/shared_ptr.hpp>

#include "grid.hpp"

template <class T>
class Interpolator {
public:
  virtual ~Interpolator() {}
  virtual T getValue(const Grid<T>& src, double x, double y) const = 0;

};

template<class T>
class NearestNeighbor : public Interpolator<T> {
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
class Bilinear : public Interpolator<T> {
public:
  T getValue(const Grid<T>& src, double x, double y) const {
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
    // TODO: Should this be rounding instead of casting?
    return static_cast<T>(linear_interp(y, y1, y2, n, s));
  }
};

template <class T>
boost::shared_ptr<Interpolator<T> > getInterpolator(const std::string& name) {
  if (name == "ngb") {
    return boost::shared_ptr<Interpolator<T> >(new NearestNeighbor<T>());
  } else if (name == "bilinear") {
    return boost::shared_ptr<Interpolator<T> >(new Bilinear<T>());
  } else {
    return boost::shared_ptr<Interpolator<T> >();
  }
}

#endif
