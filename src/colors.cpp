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


// === BEGIN SRGB/LAB CONVERSION =======================================

double linear2srgb(double c) {
  double a = 0.055;
  if (c <= 0.0031308) {
    return 12.92 * c;
  } else {
    return (1 + a) * ::pow(c, 1.0/2.4) - a;
  }
}

double srgb2linear(double c) {
  double a = 0.055;
  if (c <= 0.04045) {
    return c / 12.92;
  } else {
    return ::pow((c + a) / (1 + a), 2.4);
  }
}

double d65_x = 0.95047;
double d65_y = 1.0;
double d65_z = 1.08883;

double srgb_xyz[][3] = {
  {0.4124564,  0.3575761,  0.1804375},
  {0.2126729,  0.7151522,  0.0721750},
  {0.0193339,  0.1191920,  0.9503041}
};

void srgb2xyz(double r, double g, double b, double* x, double *y, double* z) {
  r = srgb2linear(r);
  g = srgb2linear(g);
  b = srgb2linear(b);
  *x = srgb_xyz[0][0] * r + srgb_xyz[0][1] * g + srgb_xyz[0][2] * b;
  *y = srgb_xyz[1][0] * r + srgb_xyz[1][1] * g + srgb_xyz[1][2] * b;
  *z = srgb_xyz[2][0] * r + srgb_xyz[2][1] * g + srgb_xyz[2][2] * b;
}

double xyz_srgb[][3] = {
  {3.2404542,  -1.5371385, -0.4985314},
  {-0.9692660,  1.8760108,  0.0415560},
  {0.0556434,  -0.2040259,  1.0572252}
};

void xyz2srgb(double x, double y, double z, double *r, double *g, double *b) {
  *r = xyz_srgb[0][0] * x + xyz_srgb[0][1] * y + xyz_srgb[0][2] * z;
  *g = xyz_srgb[1][0] * x + xyz_srgb[1][1] * y + xyz_srgb[1][2] * z;
  *b = xyz_srgb[2][0] * x + xyz_srgb[2][1] * y + xyz_srgb[2][2] * z;
  *r = linear2srgb(*r);
  *g = linear2srgb(*g);
  *b = linear2srgb(*b);
}

double labf(double t) {
  if (t > ::pow(6.0 / 29.0, 3.0)) {
    return ::pow(t, 1.0 / 3.0);
  } else {
    return 1.0/3.0 * ::pow(29.0 / 6.0, 2.0) * t + (4.0 / 29.0);
  }
}

void xyz2lab(double x, double y, double z, double *l, double *a, double *b) {
  x = x / d65_x;
  y = y / d65_y;
  z = z / d65_z;
  *l = 116.0 * labf(y) - 16.0;
  *a = 500.0 * (labf(x) - labf(y));
  *b = 200.0 * (labf(y) - labf(z));
}

double labf_inv(double t) {
  if (t > 6.0 / 29.0) {
    return ::pow(t, 3.0);
  } else {
    return 3 * ::pow(6.0/29.0, 2) * (t - 4.0 / 29.0);
  }
}

void lab2xyz(double l, double a, double b, double *x, double *y, double *z) {
  *y = d65_y * labf_inv(1.0 / 116.0 * (l + 16.0));
  *x = d65_x * labf_inv(1.0 / 116.0 * (l + 16.0) + 1.0 / 500.0 * a);
  *z = d65_z * labf_inv(1.0 / 116.0 * (l + 16.0) - 1.0 / 200.0 * b);
}

void srgb2lab(double red, double green, double blue, double *l, double *a, double *b) {
  double x, y, z;
  srgb2xyz(red, green, blue, &x, &y, &z);
  xyz2lab(x, y, z, l, a, b);
}
void lab2srgb(double l, double a, double b, double *red, double *green, double *blue) {
  double x, y, z;
  lab2xyz(l, a, b, &x, &y, &z);
  xyz2srgb(x, y, z, red, green, blue);
}

// === END SRGB/LAB CONVERSION =======================================


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
        double l, a, b;
        double opacity = 0;
        if (colorOffset == ncolors - 1) {
          // xvalue is exactly at the top of the range. Just use the top color.
          l = colors(0, colorOffset);
          a = colors(1, colorOffset);
          b = colors(2, colorOffset);
          if (alpha) {
            opacity = colors(3, colorOffset);
          }
        } else {
          // Do a linear interp between the two closest colors.
          double factorB = xval - colorOffset;
          double factorA = 1 - factorB;
          l = factorA * colors(0, colorOffset) + factorB * colors(0, colorOffset + 1);
          a = factorA * colors(1, colorOffset) + factorB * colors(1, colorOffset + 1);
          b = factorA * colors(2, colorOffset) + factorB * colors(2, colorOffset + 1);
          if (alpha) {
            opacity = ::round(factorA * colors(3, colorOffset) + factorB * colors(3, colorOffset + 1));
          }
        }

        double red, green, blue;
        lab2srgb(l, a, b, &red, &green, &blue);
        red = ::round(red * 255);
        green = ::round(green * 255);
        blue = ::round(blue * 255);

        // Convert the result to hex string
        if (!alpha)
          result[i] = rgbcolor(red, green, blue);
        else
          result[i] = rgbacolor(red, green, blue, opacity);
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
  for (int col = 0; col < colors.cols(); col++) {
    double red = colors(0, col) / 255;
    double green = colors(1, col) / 255;
    double blue = colors(2, col) / 255;
    double l, a, b;
    srgb2lab(red, green, blue, &l, &a, &b);
    colors(0, col) = l;
    colors(1, col) = a;
    colors(2, col) = b;
  }
  return doColorRampParallel(colors, x, alpha, naColor);
}

// For unit testing
// [[Rcpp::export]]
NumericVector rgbToLab(NumericVector rgb) {
  double l, a, b;
  srgb2lab(rgb[0], rgb[1], rgb[2], &l, &a, &b);
  NumericVector result(3);
  result[0] = l;
  result[1] = a;
  result[2] = b;
  return result;
}

// For unit testing
// [[Rcpp::export]]
NumericVector rgbToXyz(NumericVector rgb) {
  double x, y, z;
  srgb2xyz(rgb[0], rgb[1], rgb[2], &x, &y, &z);
  NumericVector result(3);
  result[0] = x;
  result[1] = y;
  result[2] = z;
  return result;
}

/*** R
x <- runif(10000)
colors <- c('black', 'white')
c0 <- function(x) {
  rgb(colorRamp(colors, space = 'Lab')(x) / 255)
}
c1 <- rasterfaster::createColorRamp(colors)

which(c0(x) != c1(x))
*/

