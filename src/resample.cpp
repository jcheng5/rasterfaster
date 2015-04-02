#include <Rcpp.h>
#include <RcppParallel.h>
#include "mmfile.hpp"
#include "grid.hpp"
#include "resample_algos.hpp"

using namespace Rcpp;

//  CharacterVector x = CharacterVector::create("foo", "bar");
//  NumericVector y   = NumericVector::create(0.0, 1.0);
//  List z            = List::create(x, y);

template <class T, class TInterpolator>
class ResampleWorker : public RcppParallel::Worker {
  Grid<T>* pSrc;
  Grid<T>* pTgt;
  const TInterpolator* pInterp;
  double xRatio;
  double yRatio;

public:
  ResampleWorker(Grid<T>* pSrc, Grid<T>* pTgt, const TInterpolator* pInterp) :
  pSrc(pSrc), pTgt(pTgt), pInterp(pInterp) {
    xRatio = static_cast<double>(pSrc->ncol()) / pTgt->ncol();
    yRatio = static_cast<double>(pSrc->nrow()) / pTgt->nrow();
  }

  void operator()(size_t begin, size_t end) {
    for (size_t i = begin; i < end; i++) {
      size_t x = i / pTgt->nrow();
      size_t y = i % pTgt->nrow();

      *pTgt->at(y, x) = static_cast<T>(pInterp->getValue(*pSrc,
        (x + 0.5) * xRatio - 0.5, (y + 0.5) * yRatio - 0.5));
    }
  }
};

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

  ResampleWorker<T, Algo> worker(&from_g, &to_g, &algo);
  RcppParallel::parallelFor(0, toRows * toCols, worker);
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
