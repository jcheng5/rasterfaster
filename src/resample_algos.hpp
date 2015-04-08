#ifndef RESAMPLE_ALGOS_HPP
#define RESAMPLE_ALGOS_HPP

#include <algorithm>
#include <iostream>

#include <boost/shared_ptr.hpp>

#include "grid.hpp"
#include "aggregate.hpp"

template <class T>
class Interpolator {
public:
  virtual ~Interpolator() {}
  virtual T getValue(const Grid<T>& src, double x, double y,
    double xRatio, double yRatio) const = 0;

};

template<class T>
class NearestNeighbor : public Interpolator<T> {
public:
  T getValue(const Grid<T>& src, double x, double y,
    double xRatio, double yRatio) const {

    return *src.at(
        static_cast<index_t>(round(y)),
        static_cast<index_t>(round(x))
    );
  }
};

template<class T>
class NeighborMode : public Interpolator<T> {
public:
  T getValue(const Grid<T>& src, double x, double y,
    double xRatio, double yRatio) const {

    index_t width = static_cast<index_t>(floor(xRatio));
    index_t height = static_cast<index_t>(floor(yRatio));

    index_t left = static_cast<index_t>(round(x)) - (width/2);
    index_t top = static_cast<index_t>(round(y)) - (height/2);
    // Dividing then multiplying by 2 is to make sure the extent is an
    // odd number, centered around round(x) or round(y).
    // One of the +1 is for the same reason.
    // The other +1 is to make right and bottom exclusive, not inclusive.
    index_t right = left + (width/2)*2 + 1 + 1;
    index_t bottom = top + (height/2)*2 + 1 + 1;

    std::vector<T> vec;
    vec.reserve(width*height);
    for (index_t y1 = top; y1 < bottom; y1++) {
      for (index_t x1 = left; x1 < right; x1++) {
        vec.push_back(*src.at(y1, x1));
      }
    }

    T result;
    if (!mode<T>(vec, &result))
      ; // what??
    return result;
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
  T getValue(const Grid<T>& src, double x, double y,
    double xRatio, double yRatio) const {

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
    //return boost::shared_ptr<Interpolator<T> >(new NearestNeighbor<T>());
    return boost::shared_ptr<Interpolator<T> >(new NeighborMode<T>());
  } else if (name == "bilinear") {
    return boost::shared_ptr<Interpolator<T> >(new Bilinear<T>());
  } else {
    return boost::shared_ptr<Interpolator<T> >();
  }
}

#endif
