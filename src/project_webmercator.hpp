#ifndef PROJECT_WEBMERCATOR_HPP
#define PROJECT_WEBMERCATOR_HPP

#include <cmath>

#include "grid.hpp"
#include "resample_algos.hpp"

using namespace Rcpp;

// Reverse-project x and y values (between 0 and 1) to lng/lat in degrees.
template <class T>
static inline void reverse_project_webmercator_one(
    double x, double y, double* lng, double* lat
  ) {

  *lng = x * 360 - 180;
  double lat_rad = atan(sinh(PI * (1 - 2*y)));
  *lat = lat_rad * 180 / PI;
}

/**
 * Create a web mercator projection of the given WGS84 data. The projection
 * can be an arbitrary resolution and aspect ratio, and can be any rectangular
 * portion of the projected data (i.e. you can make a map tile without
 * projecting the entire map first).
 *
 * @param interp The interpolation implementation to use.
 * @param src The source of the WGS84 data; may or may not be a complete
 *   360-by-180 degrees. If the requested data is not available, the closest
 *   value will be used(it should probably be NA instead).
 * @param lat1,lat2 Minimum and maximum latitude present in the src.
 * @param lng1,lng2 Minimum and maximum longitude present in the src.
 * @param tgt The target of the projection.
 * @param xOrigin,xTotal,yOrigin,yTotal If the entire 360-by-180 degree world
 *   projected is xTotal by yTotal pixels, the tgt is a square located at
 *   xOrigin and yOrigin.
 */
template <class T, class TInterpolator>
void project_webmercator(TInterpolator interp,
  const Grid<T>& src, double lat1, double lat2, double lng1, double lng2,
  const Grid<T>& tgt, index_t xOrigin, index_t xTotal, index_t yOrigin, index_t yTotal) {

  for (index_t y = 0; y < tgt.nrow(); y++) {
    for (index_t x = 0; x < tgt.ncol(); x++) {
      double lng, lat;

      double xNorm = (static_cast<double>(x) + xOrigin) / xTotal;
      double yNorm = (static_cast<double>(y) + yOrigin) / yTotal;

      reverse_project_webmercator_one<T>(xNorm, yNorm, &lng, &lat);

      double srcX = (lng + 180) / 360 * src.ncol();
      double srcY = (1 - (lat + 90) / 180) * src.nrow();

      *tgt.at(y, x) = interp.getValue(src, srcX, srcY);
    }
  }
}

#endif
