# rasterfaster

This package is intended to provide faster but potentially less general/flexible raster operations than are available in the `raster` package itself.

Features:

1. Resampling - nearest neighbor (implemented) and bilinear (in progress)
2. Projection (not implemented)

Currently only `.grd` files (as created by `raster::writeRaster`) with `numeric` data are supported.

**Don't even think about using this package for any real analysis yet.**

## Usage

```r
r <- raster("yourfile.grd")
plot(resampleBy(r, 2.3))  # 2.3X larger
plot(resampleTo(r, 90, 180))  # 90 rows by 180 columns
```

## Installation

```r
devtools::install_github("jcheng5/rasterfaster")
```

## License

GPL >=2
