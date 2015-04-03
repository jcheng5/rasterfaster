#include <Rcpp.h>
// [[Rcpp::depends(RcppParallel)]]
#include <RcppParallel.h>

#include "mmfile.hpp"
#include "grid.hpp"
#include "resample_algos.hpp"
#include "project_webmercator.hpp"

template <class T>
void project_files(
    const std::string& name,
    const std::string& method,
    const std::string& from, index_t fromStride, index_t fromRows, index_t fromCols,
    int lng1, int lng2, int lat1, int lat2,
    const std::string& to, index_t toStride, index_t toRows, index_t toCols,
    index_t x, index_t y, index_t totalWidth, index_t totalHeight
) {
  boost::shared_ptr<Projection<T> > pProject = getProjection<T>(name);
  if (!pProject) {
    Rcpp::stop("Unsupported projection: %s", name);
  }
  boost::shared_ptr<Interpolator<T> > pInterp = getInterpolator<T>(method);
  if (!pInterp) {
    Rcpp::stop("Unsupported interpolator: %s", method);
  }

  // Memory mapped files
  MMFile<T> from_f(from, boost::interprocess::read_only);
  MMFile<T> to_f(to, boost::interprocess::read_write);

  // Grid will help us conveniently offset into mmap by row/col
  Grid<T> from_g(from_f.begin(), from_f.end(), fromStride, fromRows, fromCols);
  Grid<T> to_g(to_f.begin(), to_f.end(), toStride, toRows, toCols);

  project<T>(pProject.get(), pInterp.get(), from_g, lat1, lat2, lng1, lng2,
    to_g, x, totalWidth, y, totalHeight);
}

// TODO: Implement dataFormat, method

// [[Rcpp::export]]
void do_project(
    const std::string& name,
    const std::string& from, int fromStride, int fromRows, int fromCols,
    int lng1, int lng2, int lat1, int lat2,
    const std::string& to, int toStride, int toRows, int toCols,
    int x, int y, int totalWidth, int totalHeight,
    const std::string& dataFormat, const std::string& method
) {
  if (dataFormat == "FLT8S") {
    project_files<double>(name, method, from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2, to, toStride, toRows, toCols, x, y, totalWidth, totalHeight);
  } else if (dataFormat == "FLT4S") {
    project_files<float>(name, method, from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2, to, toStride, toRows, toCols, x, y, totalWidth, totalHeight);
  } else if (dataFormat == "INT4U") {
    project_files<uint32_t>(name, method, from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2, to, toStride, toRows, toCols, x, y, totalWidth, totalHeight);
  } else if (dataFormat == "INT4S") {
    project_files<int32_t>(name, method, from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2, to, toStride, toRows, toCols, x, y, totalWidth, totalHeight);
  } else if (dataFormat == "INT2U") {
    project_files<uint16_t>(name, method, from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2, to, toStride, toRows, toCols, x, y, totalWidth, totalHeight);
  } else if (dataFormat == "INT2S") {
    project_files<int16_t>(name, method, from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2, to, toStride, toRows, toCols, x, y, totalWidth, totalHeight);
  } else if (dataFormat == "INT1U") {
    project_files<uint8_t>(name, method, from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2, to, toStride, toRows, toCols, x, y, totalWidth, totalHeight);
  } else if (dataFormat == "INT1S") {
    project_files<int8_t>(name, method, from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2, to, toStride, toRows, toCols, x, y, totalWidth, totalHeight);
  } else if (dataFormat == "LOG1S") {
    if (sizeof(bool) != 1) {
      Rcpp::stop("The size of 'bool' on your architecture is not 1 byte. Please report this issue to the rasterfaster author.");
    }
    project_files<bool>(name, method, from, fromStride, fromRows, fromCols, lng1, lng2, lat1, lat2, to, toStride, toRows, toCols, x, y, totalWidth, totalHeight);
  } else {
    Rcpp::stop("Unknown data format: %s", dataFormat);
  }
}
