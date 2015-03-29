#ifndef RESAMPLE_ALGOS_HPP
#define RESAMPLE_ALGOS_HPP

#include <algorithm>
#include <iostream>
#include "grid.hpp"

template<class T>
class NearestNeighbor {
public:
  void resample(const Grid<T>& src, const Grid<T>& tgt) const {

    float xRatio = static_cast<float>(src.ncol()) / tgt.ncol();
    float yRatio = static_cast<float>(src.nrow()) / tgt.nrow();

    index_t i = 0;

    for (index_t toCol = 0; toCol < tgt.ncol(); toCol++) {
      for (index_t toRow = 0; toRow < tgt.nrow(); toRow++) {
        if ((++i % 10000) == 0) {
          Rcpp::checkUserInterrupt();
        }

        // nearest neighbor
        *tgt.at(toRow, toCol) = *src.at(
          static_cast<index_t>(toRow * yRatio),
          static_cast<index_t>(toCol * xRatio)
        );
      }
    }
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
  void resample(const Grid<T>& src, const Grid<T>& tgt) const {

    double xRatio = static_cast<double>(src.ncol()) / tgt.ncol();
    double yRatio = static_cast<double>(src.nrow()) / tgt.nrow();

    index_t i = 0;

    for (index_t x = 0; x < tgt.ncol(); x++) {
      for (index_t y = 0; y < tgt.nrow(); y++) {

        if ((++i % 10000) == 0) {
          Rcpp::checkUserInterrupt();
        }

        double srcX = xRatio * x - 0.5;
        double srcY = yRatio * y - 0.5;

        //std::cout << srcX << "," << srcY << "\n";

        index_t x1 = std::floor(srcX), x2 = std::ceil(srcX);
        index_t y1 = std::floor(srcY), y2 = std::ceil(srcY);

        // Get the values at the four pixels surrounding srcX/srcY.
        double nw = *src.at(y1, x1);
        double ne = *src.at(y1, x2);
        double sw = *src.at(y2, x1);
        double se = *src.at(y2, x2);

        // Combine the two northern points using linear interpolation.
        double n = linear_interp(srcX, x1, x2, nw, ne);
        // Combine the two southern points using linear interpolation.
        double s = linear_interp(srcX, x1, x2, sw, se);
        // Combine the calculated north and south values.
        double result = linear_interp(srcY, y1, y2, n, s);

        //std::cout << "nw: " << nw << " ne: " << ne << " sw: " << sw << " se: " << se << " n: " << n << " s: " << s << " result: " << result << "\n";

        // Set the value.
        *tgt.at(y, x) = static_cast<T>(result);  // TODO: round instead of cast??
      }
    }
  }
};

#endif

