#include <Rcpp.h>
// [[Rcpp::depends(RcppParallel)]]
#include <RcppParallel.h>

#include <iomanip>
#include <sstream>
#include <iostream>

using namespace Rcpp;
using namespace RcppParallel;

// Convert an integer (0-255) to two ASCII hex digits, starting at buf
void intToHex(unsigned int x, char* buf) {
  const char* hexchars = "0123456789ABCDEF";
  buf[0] = hexchars[(x >> 4) & 0xF];
  buf[1] = hexchars[x & 0xF];
}

// Convert the rgb values to #RRGGBB hex string
std::string rgbcolor(double r, double g, double b) {
  char color[8];
  color[0] = '#';
  intToHex(static_cast<unsigned int>(r), color + 1);
  intToHex(static_cast<unsigned int>(g), color + 3);
  intToHex(static_cast<unsigned int>(b), color + 5);
  color[7] = 0;
  return std::string(color);
}

// Convert the rgba values to #RRGGBB hex string
std::string rgbacolor(double r, double g, double b, double a) {
  char color[10];
  color[0] = '#';
  intToHex(static_cast<unsigned int>(r), color + 1);
  intToHex(static_cast<unsigned int>(g), color + 3);
  intToHex(static_cast<unsigned int>(b), color + 5);
  intToHex(static_cast<unsigned int>(a), color + 7);
  color[9] = 0;
  return std::string(color);
}

class ColorRampWorker : public RcppParallel::Worker {
  // inputs
  const RMatrix<double> colors;
  const RVector<double> x;
  const bool alpha;
  const size_t ncolors;

public:
  // output
  tbb::concurrent_vector<std::string> result;

  ColorRampWorker(const RMatrix<double> colors, const RVector<double> x, bool alpha)
    : colors(colors), x(x), alpha(alpha), ncolors(colors.ncol()), result(x.length()) {
  }

  void operator()(std::size_t begin, std::size_t end) {
    for (size_t i = begin; i < end; i++) {
      double xval = x[i];
      if (xval < 0 || xval > 1 || R_IsNA(xval)) {
        // Illegal or NA value for this x value. We can't use NA here but "" will
        // be replaced with NA later, when we're back on the R thread.
        result[i] = std::string();
      } else {
        // Scale the [0,1] value to [0,n-1]
        xval *= ncolors - 1;
        // Find the closest color that's *lower* than xval. This'll be one of the
        // colors we use to interpolate; the other will be colorOffset+1.
        size_t colorOffset = static_cast<size_t>(::floor(xval));
        double r, g, b;
        double a = 0;
        if (colorOffset == ncolors - 1) {
          // xvalue is exactly at the top of the range. Just use the top color.
          r = colors(0, colorOffset);
          g = colors(1, colorOffset);
          b = colors(2, colorOffset);
          if (alpha) {
            a = colors(3, colorOffset);
          }
        } else {
          // Do a linear interp between the two closest colors.
          double factorB = xval - colorOffset;
          double factorA = 1 - factorB;
          r = ::round(factorA * colors(0, colorOffset) + factorB * colors(0, colorOffset + 1));
          g = ::round(factorA * colors(1, colorOffset) + factorB * colors(1, colorOffset + 1));
          b = ::round(factorA * colors(2, colorOffset) + factorB * colors(2, colorOffset + 1));
          if (alpha) {
            a = ::round(factorA * colors(3, colorOffset) + factorB * colors(3, colorOffset + 1));
          }
        }

        // Convert the result to hex string
        if (!alpha)
          result[i] = rgbcolor(r, g, b);
        else
          result[i] = rgbacolor(r, g, b, a);
      }
    }
  }
};

StringVector doColorRampParallel(NumericMatrix colors, NumericVector x, bool alpha, std::string naColor) {
  // We can't use normal Rcpp data structures on other threads, so use
  // RcppParallel-provided matrix and vector classes instead.
  RMatrix<double> rcolors(colors);
  RVector<double> rx(x);
  ColorRampWorker crw(rcolors, rx, alpha);
  parallelFor(0, x.size(), crw);

  // Copy the results from ColorRampWorker's tbb::concurrent_vector<std::string>
  // to a StringVector that's suitable for returning to R.
  // TODO: Is there a way to speed this up or do fewer allocations? Last I
  //   checked we're now spending 50% of our time in this loop.
  StringVector result(x.length());
  for (size_t i = 0; i < x.length(); i++) {
    if (crw.result[i].empty()) {
      if (naColor.empty())
        result[i] = NA_STRING;
      else
        result[i] = naColor;
    } else {
      result[i] = crw.result[i];
    }
  }
  return result;
}

// Same as doColorRampParallel, but doesn't use multiple cores. Not that much
// slower than the parallel version, actually.
StringVector doColorRampSerial(NumericMatrix colors, NumericVector x, bool alpha, std::string naColor) {

  size_t ncolors = colors.ncol();

  StringVector result(x.length());
  for (size_t i = 0; i < x.length(); i++) {
    double xval = x[i];
    if (xval < 0 || xval > 1 || R_IsNA(xval)) {
      if (naColor.empty())
        result[i] = NA_STRING;
      else
        result[i] = naColor;
    } else {
      xval *= ncolors - 1;
      size_t colorOffset = static_cast<size_t>(::floor(xval));
      double r, g, b;
      double a = 0;
      if (colorOffset == ncolors - 1) {
        // Just use the top color
        r = colors(0, colorOffset);
        g = colors(1, colorOffset);
        b = colors(2, colorOffset);
        if (alpha) {
          a = colors(3, colorOffset);
        }
      } else {
        // Do a linear interp between the two closest colors
        double factorB = xval - colorOffset;
        double factorA = colorOffset + 1 - xval;
        r = ::round(factorA * colors(0, colorOffset) + factorB * colors(0, colorOffset + 1));
        g = ::round(factorA * colors(1, colorOffset) + factorB * colors(1, colorOffset + 1));
        b = ::round(factorA * colors(2, colorOffset) + factorB * colors(2, colorOffset + 1));
        if (alpha) {
          a = ::round(factorA * colors(3, colorOffset) + factorB * colors(3, colorOffset + 1));
        }
      }

      if (!alpha)
        result[i] = rgbcolor(r, g, b);
      else
        result[i] = rgbacolor(r, g, b, a);
    }
  }
  return result;
}

// [[Rcpp::export]]
StringVector doColorRamp(NumericMatrix colors, NumericVector x, bool alpha, std::string naColor) {
  return doColorRampParallel(colors, x, alpha, naColor);
}
