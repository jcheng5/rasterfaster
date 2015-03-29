# rasterfaster

This package is intended to provide faster but potentially less general/flexible raster operations than are available in the `raster` package itself. One of the primary goals is to enable near-realtime exploration of large raster data files with [Leaflet](https://github.com/rstudio/leaflet).

In particular, `raster::resample(method="ngb")` is strangely slow for large `.grd` files, like this 7200x3600 example:

```r
> system.time(raster::resample(r, raster(nrow=100, ncol=100), method="ngb"))
   user  system elapsed 
 29.729   1.941  31.809 
> system.time(rasterfaster::resampleTo(r, 100, 100, method="ngb"))
   user  system elapsed 
  0.032   0.005   0.053
```

The gap is much narrower so far for bilinear:

```r
> system.time(raster::resample(r, raster(nrow=100, ncol=100), method='bilinear'))
   user  system elapsed 
  2.151   0.264   2.628 
> system.time(rasterfaster::resampleTo(r, 100, 100, method="bilinear"))
   user  system elapsed 
  0.031   0.014   0.138 
```

Take these benchmarks with a huge grain of salt, as the performance will undoubtedly change over time.

## Features

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
