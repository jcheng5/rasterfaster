#' @import raster
#' @useDynLib rasterfaster
#' @importFrom Rcpp evalCpp
#' @importFrom RcppParallel RcppParallelLibs

grdToGri <- function(filename) {
  sub("\\.grd$", ".gri", filename)
}

# Force a file to be exactly the specified length, expanding or
# truncating the file as necessary.
forceFileToLength <- function(filename, length) {
  f <- file(filename, open = "wb")
  tryCatch(
    {
      if (length > 0) {
        seek(f, length)
      }
      truncate(f)
    },
    finally = close(f)
  )
  invisible()
}

verifyInputRaster <- function(x, labelForError) {
  if (!inherits(x, "RasterLayer")) {
    stop(labelForError, " only works on RasterLayer objects")
  }
  if (!identical(x@file@driver, "raster")) {
    stop(labelForError, " only works on .grd raster files")
  }
  if (!inherits(x[1, 1], "numeric")) {
    stop(labelForError, " only works on numeric values")
  }
}

createOutputGrdFile <- function(x, y, filename = tempfile(fileext = ".grd")) {
  if (!isTRUE(grepl("\\.grd", filename))) {
    stop("Output filename must have .grd extension")
  }

  # Create the .grd header and a dummy (empty) .gri file
  wh <- writeStart(y, filename, datatype = dataType(y))
  suppressWarnings(
    writeStop(wh)  # Rightfully warns about data not being present
  )

  dataWidth <- switch(dataType(y),
    FLT8S = 8,
    FLT4S = 4,
    INT4U = 4,
    INT4S = 4,
    INT2U = 2,
    INT2S = 2,
    INT1U = 1,
    INT1S = 1,
    LOG1S = 1,
    stop("Unsupported data notation ", dataType(y))
  )

  outsize <- ncell(y) * dataWidth
  forceFileToLength(grdToGri(filename), outsize)
  filename
}

resampleLayer <- function(x, y, method = c("bilinear", "ngb")) {
  method <- match.arg(method)

  verifyInputRaster(x, "resampleLayer")
  outfile <- createOutputGrdFile(x, y)
  inFile <- grdToGri(x@file@name)

  resample_files_numeric(inFile, raster::ncol(x), raster::nrow(x), raster::ncol(x),
    grdToGri(outfile), raster::ncol(y), raster::nrow(y), raster::ncol(y),
    x@file@datanotation, method
  )

  result <- raster(outfile)
  result@data@haveminmax <- FALSE
  result
}

#' Resample a numeric RasterLayer
#'
#' @param x RasterLayer object to be resampled. Currently it MUST be backed by a
#'   .grd file and must have \code{numeric} data.
#' @param factor Factor to resize by (for example, \code{0.5} for 50\%,
#'   \code{3.2} for 320\%).
#' @param nrow,ncol Number of rows and columns in the output layer.
#' @param method \code{"bilinear"} for bilinear interpolation, or \code{"ngb"}
#'   for nearest-neighbor.
#' @return Resampled raster.
#' @examples
#' library(raster)
#' src <- raster(system.file("sample.grd", package = "rasterfaster"))
#' plot(src)
#' system.time(result <- resampleBy(src, 8.4))
#' plot(result)
#' @export
resampleBy <- function(x, factor, method = c("bilinear", "ngb")) {
  method <- match.arg(method)

  y <- x
  nrow(y) <- ceiling(nrow(x) * factor)
  ncol(y) <- ceiling(nrow(y) * factor)
  resampleLayer(x, y, method)
}

#' @rdname resampleBy
#' @export
resampleTo <- function(x, nrow = 180, ncol = 360, method = c("bilinear", "ngb")) {
  method <- match.arg(method)

  y <- x
  nrow(y) <- nrow
  ncol(y) <- ncol
  resampleLayer(x, y, method)
}

#' Create a web map tile
#'
#' @param x A \code{Raster} object (as created by \code{raster::raster()}) with
#'   unprojected WGS84 data. It's not required to contain the entire 360-by-180
#'   degree world.
#' @param width The width of the tile to create.
#' @param height The height of the tile to create.
#' @param xtile The x-number of the tile.
#' @param ytile The y-number of the tile.
#' @param zoom The zoom level of the tile.
#' @param method The type of interpolation to use. \code{"auto"} (the default)
#'   means bilinear when reducing, and nearest neighbor when enlarging.
#'
#' @return A \code{Raster} object.
#'
#' @export
createMapTile <- function(x, width, height, xtile, ytile, zoom,
  projection = c("epsg:3857"), method = c("auto", "bilinear", "ngb")) {

  projection <- match.arg(projection)
  method <- match.arg(method)

  # TODO: Validate parameters

  y <- x
  raster::ncol(y) <- width
  raster::nrow(y) <- height
  xmin(y) <- 0
  ymin(y) <- 0
  xmax(y) <- width
  ymax(y) <- height

  verifyInputRaster(x, "createMapTile")
  outfile <- createOutputGrdFile(x, y)
  inFile <- grdToGri(x@file@name)

  if (identical(method, "auto")) {
    # Determine if source resolution is greater than target resolution, so we
    # can use bilinear to reduce, but nearest neighbor to enlarge.
    srcResX <- raster::ncol(x) / (xmax(x) - xmin(x))
    srcResY <- raster::nrow(x) / (ymax(x) - ymin(x))
    tgtResX <- 2^zoom * width / 360
    tgtResY <- 2^zoom * height / 180
    method <- if (srcResX >= tgtResX || srcResY >= tgtResY) {
      "bilinear"
    } else {
      "ngb"
    }
  }

  do_project(projection, inFile, raster::ncol(x), raster::nrow(x), raster::ncol(x),
    xmin(x), xmax(x), ymin(x), ymax(x),
    grdToGri(outfile), raster::ncol(y), raster::nrow(y), raster::ncol(y),
    xtile * width, ytile * height, 2^zoom * width, 2^zoom * height,
    x@file@datanotation, method
  )

  result <- raster(outfile)
  crs(result) <- sp::CRS("+init=epsg:3857 +proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +no_defs")
  result@data@haveminmax <- FALSE
  result
}

quote({
library(rasterfaster);library(raster);library(digest);library(testthat)
system.time(r <- resampleBy(raster("testdata/shipping.grd"), 0.5)); plot(r)
system.time(r <- resampleTo(raster("testdata/shipping.grd"), method = "ngb")); plot(r); expect_equal(digest(values(r)), "6b77f1061cada94b4d4fcf687b0f0cb1")
system.time(r <- resampleTo(raster("testdata/shipping.grd"), 360, 720, method = "bilinear")); plot(r); expect_equal(digest(values(r)), "7cc22b6b7513cad33f05df65336ddcc1")
plot(raster("testdata/acid.grd"))
system.time(r <- resampleTo(raster("testdata/acid.grd"), method = "ngb")); plot(r); expect_equal(digest(values(r)), "96910b8d56b4e0391e04ad63b7743c9b")
system.time(r <- resampleTo(raster("testdata/acid.grd"), method = "bilinear")); plot(r); expect_equal(digest(values(r)), "04b2c247ed6388aa2be1e5561d3f31c9")
})
