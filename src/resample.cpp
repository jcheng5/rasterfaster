#include <boost/cstdint.hpp>
#include <Rcpp.h>
// [[Rcpp::depends(RcppParallel)]]
#include <RcppParallel.h>
#include "mmfile.hpp"
#include "grid.hpp"
#include "resample_algos.hpp"

using namespace Rcpp;

//  CharacterVector x = CharacterVector::create("foo", "bar");
//  NumericVector y   = NumericVector::create(0.0, 1.0);
//  List z            = List::create(x, y);

template <class T>
class ResampleWorker : public RcppParallel::Worker {
  Grid<T>* pSrc;
  Grid<T>* pTgt;
  const Interpolator<T>* pInterp;
  double xRatio;
  double yRatio;

public:
  ResampleWorker(Grid<T>* pSrc, Grid<T>* pTgt, const Interpolator<T>* pInterp) :
  pSrc(pSrc), pTgt(pTgt), pInterp(pInterp) {
    xRatio = static_cast<double>(pSrc->ncol()) / pTgt->ncol();
    yRatio = static_cast<double>(pSrc->nrow()) / pTgt->nrow();
  }

  void operator()(size_t begin, size_t end) {
    for (size_t i = begin; i < end; i++) {
      size_t x = i / pTgt->nrow();
      size_t y = i % pTgt->nrow();

      *pTgt->at(y, x) = static_cast<T>(pInterp->getValue(*pSrc,
        (x + 0.5) * xRatio - 0.5, (y + 0.5) * yRatio - 0.5,
        xRatio, yRatio));
    }
  }
};

template<class T>
void resample_files(const std::string& method,
  const std::string& from, index_t fromStride, index_t fromRows, index_t fromCols,
  const std::string& to, index_t toStride, index_t toRows, index_t toCols) {

  boost::shared_ptr<Interpolator<T> > interp = getInterpolator<T>(method);
  if (!interp) {
    Rcpp::stop("Unknown resampling method %s", method);
  }

  // Memory mapped files
  MMFile<T> from_f(from, boost::interprocess::read_only);
  MMFile<T> to_f(to, boost::interprocess::read_write);

  // Grid will help us conveniently offset into mmap by row/col
  Grid<T> from_g(from_f.begin(), from_f.end(), fromStride, fromRows, fromCols);
  Grid<T> to_g(to_f.begin(), to_f.end(), toStride, toRows, toCols);

  ResampleWorker<T> worker(&from_g, &to_g, interp.get());
  RcppParallel::parallelFor(0, toRows * toCols, worker);
}

// [[Rcpp::export]]
void resample_files_numeric(
    const std::string& from, int fromStride, int fromRows, int fromCols,
    const std::string& to, int toStride, int toRows, int toCols,
    const std::string& dataFormat,
    const std::string& method) {

  if (dataFormat == "FLT8S") {
    resample_files<double>(method, from, fromStride, fromRows, fromCols, to, toStride, toRows, toCols);
  } else if (dataFormat == "FLT4S") {
    resample_files<float>(method, from, fromStride, fromRows, fromCols, to, toStride, toRows, toCols);
  } else if (dataFormat == "INT4U") {
    resample_files<uint32_t>(method, from, fromStride, fromRows, fromCols, to, toStride, toRows, toCols);
  } else if (dataFormat == "INT4S") {
    resample_files<int32_t>(method, from, fromStride, fromRows, fromCols, to, toStride, toRows, toCols);
  } else if (dataFormat == "INT2U") {
    resample_files<uint16_t>(method, from, fromStride, fromRows, fromCols, to, toStride, toRows, toCols);
  } else if (dataFormat == "INT2S") {
    resample_files<int16_t>(method, from, fromStride, fromRows, fromCols, to, toStride, toRows, toCols);
  } else if (dataFormat == "INT1U") {
    resample_files<uint8_t>(method, from, fromStride, fromRows, fromCols, to, toStride, toRows, toCols);
  } else if (dataFormat == "INT1S") {
    resample_files<int8_t>(method, from, fromStride, fromRows, fromCols, to, toStride, toRows, toCols);
  } else if (dataFormat == "LOG1S") {
    if (sizeof(bool) != 1) {
      Rcpp::stop("The size of 'bool' on your architecture is not 1 byte. Please report this issue to the rasterfaster author.");
    }
    resample_files<bool>(method, from, fromStride, fromRows, fromCols, to, toStride, toRows, toCols);
  } else {
    Rcpp::stop("Unknown data format: %s", dataFormat);
  }
}
