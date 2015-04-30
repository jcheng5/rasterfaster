# rasterfaster

This package is intended to provide faster but potentially less general/flexible raster operations than are available in the `raster` package itself. One of the primary goals is to enable near-realtime exploration of large raster data files with [Leaflet](https://github.com/rstudio/leaflet).

## Features

1. Resampling (nearest neighbor and bilinear)
2. WGS84 to Web Mercator or Mollweide projection, and map tile extraction

Currently only `.grd` files (as created by `raster::writeRaster`) with `numeric` data are supported.

**Don't even think about using this package for any real analysis yet.**

## Usage

```r
r <- raster("yourfile.grd")
plot(resampleBy(r, 2.3))  # 2.3X larger
plot(resampleTo(r, 90, 180))  # 90 rows by 180 columns
createMapTile(r, 256, 256, xtile = 2624, ytile = 5719, zoom = 14,
  projection = "epsg:3857", method = "auto")
```

## Installation

```r
devtools::install_github("jcheng5/rasterfaster")
```

## License

GPL >=2
