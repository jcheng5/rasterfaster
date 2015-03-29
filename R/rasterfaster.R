#' @import raster
#' @useDynLib rasterfaster
#' @importFrom Rcpp evalCpp

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

resampleLayer <- function(x, y, method = c("bilinear", "ngb")) {
  method <- match.arg(method)

  if (!inherits(x, "RasterLayer")) {
    stop("resampleLayer only works on RasterLayer objects")
  }
  if (!identical(x@file@driver, "raster")) {
    stop("resampleLayer only works on .grd raster files")
  }
  if (!inherits(x[1, 1], "numeric")) {
    stop("resampleLayer only works on numeric values")
  }
  if (!isTRUE(x@file@datanotation %in% c("FLT4S", "FLT8S"))) {
    stop("resampleLayer only works on double-precision raster files")
  }

  # Create the .grd header and a dummy (empty) .gri file
  outfile <- tempfile(fileext = ".grd")
  wh <- writeStart(y, outfile)
  suppressWarnings(
    writeStop(wh)  # Rightfully warns about data not being present
  )

  dataWidth <- switch(x@file@datanotation,
    FLT4S = 4,
    FLT8S = 8,
    stop("Unsupported data notation ", x@file@datanotation)
  )

  outsize <- ncell(y) * dataWidth
  forceFileToLength(grdToGri(outfile), outsize)

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

quote({
library(rasterfaster);library(raster);library(digest);library(testthat)
system.time(r <- resampleBy(raster("shipping.grd"), 0.5)); plot(r)
system.time(r <- resampleTo(raster("shipping.grd"), method = "ngb")); plot(r); expect_equal(digest(values(r)), "6b77f1061cada94b4d4fcf687b0f0cb1")
system.time(r <- resampleTo(raster("shipping.grd"), 360, 720, method = "bilinear")); plot(r); expect_equal(digest(values(r)), "7cc22b6b7513cad33f05df65336ddcc1")
plot(raster("acid.grd"))
system.time(r <- resampleTo(raster("acid.grd"), method = "ngb")); plot(r); expect_equal(digest(values(r)), "96910b8d56b4e0391e04ad63b7743c9b")
system.time(r <- resampleTo(raster("acid.grd"), method = "bilinear")); plot(r); expect_equal(digest(values(r)), "04b2c247ed6388aa2be1e5561d3f31c9")
})
