#ifndef PROJECT_WEBMERCATOR_HPP
#define PROJECT_WEBMERCATOR_HPP

#include <cmath>

#include <RcppParallel.h>

#include "grid.hpp"
#include "resample_algos.hpp"

using namespace Rcpp;

template <class T>
class Projection {
public:
  virtual ~Projection() {}

  virtual void reverse(double x, double y, double* lng, double* lat) = 0;
};

template <class T>
class WebMercatorProjection : public Projection<T> {
public:
  // Reverse-project x and y values (between 0 and 1) to lng/lat in degrees.
  void reverse(double x, double y, double* lng, double* lat) {
    *lng = x * 360 - 180;
    double lat_rad = atan(sinh(PI * (1 - 2*y)));
    *lat = lat_rad * 180 / PI;
  }
};

template <class T>
class ProjectionWorker : public RcppParallel::Worker {
  Projection<T>* pProj;
  Interpolator<T>* pInterp;
  const Grid<T>* pSrc;
  double lat1, lat2, lng1, lng2;
  const Grid<T>* pTgt;
  index_t xOrigin, xTotal, yOrigin, yTotal;

public:
  ProjectionWorker(Projection<T>* pProj, Interpolator<T>* pInterp,
    const Grid<T>* pSrc, double lat1, double lat2, double lng1, double lng2,
    const Grid<T>* pTgt, index_t xOrigin, index_t xTotal, index_t yOrigin, index_t yTotal
  ) : pProj(pProj), pInterp(pInterp), pSrc(pSrc), lat1(lat1), lat2(lat2), lng1(lng1), lng2(lng2),
      pTgt(pTgt), xOrigin(xOrigin), xTotal(xTotal), yOrigin(yOrigin), yTotal(yTotal) {
  }

  void operator()(size_t begin, size_t end) {
    for (size_t i = begin; i < end; i++) {
      size_t x = i / pTgt->nrow();
      size_t y = i % pTgt->nrow();

      double lng, lat;

      double xNorm = (static_cast<double>(x) + xOrigin) / xTotal;
      double yNorm = (static_cast<double>(y) + yOrigin) / yTotal;

      pProj->reverse(xNorm, yNorm, &lng, &lat);

      double srcX = (lng + 180) / 360 * pSrc->ncol();
      double srcY = (1 - (lat + 90) / 180) * pSrc->nrow();

      *pTgt->at(y, x) = pInterp->getValue(*pSrc, srcX, srcY);
    }
  }
};

template <class T>
boost::shared_ptr<Projection<T> > getProjection(const std::string& name) {
  if (name == "epsg:3857") {
    return boost::shared_ptr<Projection<T> >(new WebMercatorProjection<T>());
  } else {
    return boost::shared_ptr<Projection<T> >();
  }
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
template <class T>
void project(Projection<T>* pProject, Interpolator<T>* pInterp,
  const Grid<T>& src, double lat1, double lat2, double lng1, double lng2,
  const Grid<T>& tgt, index_t xOrigin, index_t xTotal, index_t yOrigin, index_t yTotal) {

  ProjectionWorker<T> worker(
      pProject, pInterp, &src, lat1, lat2, lng1, lng2,
      &tgt, xOrigin, xTotal, yOrigin, yTotal);

  RcppParallel::parallelFor(0, tgt.nrow() * tgt.ncol(), worker, 256);
}

#endif
