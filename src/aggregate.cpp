#include <Rcpp.h>
#include "aggregate.hpp"

using namespace Rcpp;

template<>
double mean(double* begin, double* end) {
  long double result = 0;
  size_t length = end - begin;

  for (double* p = begin; p != end; p++) {
    result += static_cast<long double>(*p);
  }
  result /= length;

  if (std::numeric_limits<long double>::infinity() != result) {
    long double strip = 0;
    for (double* p = begin; p != end; p++) {
      strip += static_cast<long double>(*p) - result;
    }
    result += strip / length;
  }

  return result;
}

template<class T>
T naValue();

template<> int32_t naValue() { return NA_INTEGER; }
template<> double naValue() { return NA_REAL; }
template<> Rcpp::String naValue() { return NA_STRING; }
template<> bool naValue() { return NA_LOGICAL; }

template <int RTYPE, class T>
Vector<RTYPE> find_mode(const Vector<RTYPE>& x) {
  if (x.size() == 0) {
    return Vector<RTYPE>::create(naValue<T>());
  }

  T result;
  if (mode(x.begin(), x.end(), &result)) {
    return Vector<RTYPE>::create(result);
  }
  Rcpp::stop("Couldn't find mode of x");
  throw;
}

// [[Rcpp::export]]
SEXP doFindMode(SEXP x) {
  switch (TYPEOF(x)) {
  case INTSXP: return find_mode<INTSXP, int32_t>(x);
  case REALSXP: return find_mode<REALSXP, double>(x);
  case STRSXP: return find_mode<STRSXP, Rcpp::String>(x);
  case LGLSXP: return find_mode<LGLSXP, bool>(x);
  }
  Rcpp::stop("findMode only works on integer, real, character, and logical vectors");
  throw;
}

template <class TVector>
double find_mean(SEXP x) {
  TVector xv(x);
  return mean(xv.begin(), xv.end());
}

// [[Rcpp::export]]
double findMean(SEXP x) {
  switch (TYPEOF(x)) {
  case INTSXP: return find_mean<IntegerVector>(x);
  case REALSXP: return find_mean<NumericVector>(x);
  }
  Rcpp::stop("findMean only works on integer and real vectors");
  throw;
}
