#include <Rcpp.h>
#include "mmfile.hpp"
#include "grid.hpp"
#include "resample_algos.hpp"

using namespace Rcpp;

//  CharacterVector x = CharacterVector::create("foo", "bar");
//  NumericVector y   = NumericVector::create(0.0, 1.0);
//  List z            = List::create(x, y);

template<class T, class Algo>
void resample_files(const Algo& algo,
  const std::string& from, index_t fromStride, index_t fromRows, index_t fromCols,
  const std::string& to, index_t toStride, index_t toRows, index_t toCols) {

  // Memory mapped files
  MMFile<T> from_f(from, boost::interprocess::read_only);
  MMFile<T> to_f(to, boost::interprocess::read_write);

  // Grid will help us conveniently offset into mmap by row/col
  Grid<T> from_g(from_f.begin(), from_f.end(), fromStride, fromRows, fromCols);
  Grid<T> to_g(to_f.begin(), to_f.end(), toStride, toRows, toCols);

  algo.resample(from_g, to_g);
}

// [[Rcpp::export]]
void resample_files_numeric(
    const std::string& from, int fromStride, int fromRows, int fromCols,
    const std::string& to, int toStride, int toRows, int toCols,
    const std::string& dataFormat,
    const std::string& method) {

  if (dataFormat == "FLT8S") {
    if (method == "bilinear") {
      Bilinear<double> bln;
      resample_files<double, Bilinear<double> >(bln, from, fromStride, fromRows, fromCols,
        to, toStride, toRows, toCols);
    } else if (method == "ngb") {
      NearestNeighbor<double> nn;
      resample_files<double, NearestNeighbor<double> >(nn, from, fromStride, fromRows, fromCols,
        to, toStride, toRows, toCols);
    } else {
      Rcpp::stop("Unknown resampling method %s", method);
    }
  } else if (dataFormat == "FLT4S") {
    if (method == "bilinear") {
      Bilinear<float> bln;
      resample_files<float, Bilinear<float> >(bln, from, fromStride, fromRows, fromCols,
        to, toStride, toRows, toCols);
    } else if (method == "ngb") {
      NearestNeighbor<float> nn;
      resample_files<float, NearestNeighbor<float> >(nn, from, fromStride, fromRows, fromCols,
        to, toStride, toRows, toCols);
    } else {
      Rcpp::stop("Unknown interpolation method %s", method);
    }
  }
}
