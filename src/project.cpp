#include <Rcpp.h>
// [[Rcpp::depends(RcppParallel)]]
#include <RcppParallel.h>

#include "mmfile.hpp"
#include "grid.hpp"
#include "resample_algos.hpp"
#include "project_webmercator.hpp"

template <class T, class TInterpolator>
void project_webmercator_files(
    TInterpolator interp,
    const std::string& from, index_t fromStride, index_t fromRows, index_t fromCols,
    int lng1, int lng2, int lat1, int lat2,
    const std::string& to, index_t toStride, index_t toRows, index_t toCols,
    index_t x, index_t y, index_t totalWidth, index_t totalHeight
) {
  // Memory mapped files
  MMFile<T> from_f(from, boost::interprocess::read_only);
  MMFile<T> to_f(to, boost::interprocess::read_write);

  // Grid will help us conveniently offset into mmap by row/col
  Grid<T> from_g(from_f.begin(), from_f.end(), fromStride, fromRows, fromCols);
  Grid<T> to_g(to_f.begin(), to_f.end(), toStride, toRows, toCols);

  project_webmercator<double, TInterpolator>(interp, from_g, lat1, lat2, lng1, lng2,
    to_g, x, totalWidth, y, totalHeight);
//
//  const Grid<T>& src, double lat1, double lat2, double lng1, double lng2,
//  const Grid<T>& tgt, index_t xOrigin, index_t xTotal, index_t yOrigin, index_t yTotal) {
}

// TODO: Implement dataFormat, method

// [[Rcpp::export]]
void project_webmercator(
    const std::string& from, int fromStride, int fromRows, int fromCols,
    int lng1, int lng2, int lat1, int lat2,
    const std::string& to, int toStride, int toRows, int toCols,
    int x, int y, int totalWidth, int totalHeight,
    const std::string& dataFormat, const std::string& method
) {
  if (method == "bilinear") {
    Bilinear<double> bln;
    project_webmercator_files<double, Bilinear<double> >(bln,
      from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2,
      to, toStride, toRows, toCols, x, y, totalWidth, totalHeight
    );
  } else if (method == "ngb") {
    NearestNeighbor<double> nn;
    project_webmercator_files<double, NearestNeighbor<double> >(nn,
      from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2,
      to, toStride, toRows, toCols, x, y, totalWidth, totalHeight
    );
  } else {
    Rcpp::stop("Unknown interpolation method %s", method);
  }
}
