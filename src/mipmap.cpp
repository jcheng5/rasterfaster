#include <Rcpp.h>
using namespace Rcpp;

typedef int dim;
typedef unsigned char subpixel;

void boxfilter(subpixel* pSrc, dim sCols, dim sRows, dim sPlanes, dim sStride,
  subpixel* pDst, dim dStride) {

  for (dim r = 0; r < sRows; r += 2) {
    for (dim c = 0; c < sCols; c += 2) {
      for (dim p = 0; p < sPlanes; p++) {
        subpixel* p_nw = pSrc + r*sStride + c*sPlanes + p;
        subpixel* p_ne = p_nw + sPlanes;
        subpixel* p_sw = p_nw + sStride;
        subpixel* p_se = p_sw + sPlanes;
        pDst[r/2 * dStride + c/2 * sPlanes + p] =
          static_cast<subpixel>((*p_nw + *p_ne + *p_sw + *p_se) / 4.0);
      }
    }

    if ((sCols % 2) == 1) {
      // If odd number of cols, copy the final pixel
      subpixel* frm = pDst + r/2 * dStride + sCols/2 * sPlanes;
      memcpy(frm + sPlanes, frm, sPlanes);
    }
  }
  if ((sRows % 2) == 1) {
    // If odd number of rows, copy the final row
    dim colsToCopy = std::ceil(sCols/2);
    memcpy(pDst + ((sRows/2+1)*dStride),
      pDst + ((sRows/2)*dStride),
      colsToCopy * sPlanes
    );
  }

}

double log_2(double value) {
  static double log2val = std::log(2);
  return std::log(value) / log2val;
}

// Return value columns: col, row, width, height
IntegerMatrix mipmapLocations(IntegerVector dx) {
  int mips = static_cast<int>(std::ceil(log_2(dx[2])));
  IntegerMatrix result(mips, 4);

  int col = dx[1];
  int width = dx[1];

  int top = -dx[2];
  int bottom = 0;

  int resultRow = -1;

  while (true) {
    int height = bottom - top;
    if (width == 1 || height == 1)
      break;

    resultRow++;

    width = static_cast<dim>(std::ceil(width/2.0));
    height = static_cast<dim>(std::ceil(height/2.0));

    top = bottom;
    bottom = top + height;

    if (bottom > dx[2]) {
      break;
    }

    result(resultRow, 0) = col;
    result(resultRow, 1) = top;
    result(resultRow, 2) = width;
    result(resultRow, 3) = height;
  }

  return result;
}

// [[Rcpp::export]]
RawVector doMipmap(RawVector x, IntegerVector dx) {

  int planes = dx[0];
  int cols = dx[1];
  int rows = dx[2];

  // Enlarge the destination column dimension to make room for mipmap.
  // If original x had odd number of columns, add an extra column.
  int destcols = static_cast<dim>(cols * 1.5) + cols % 2;

  Rcpp::Dimension dy(planes, destcols, rows);
  Rcpp::RawVector y(dy);

  for (dim r = 0; r < rows; r++) {
    dim srcRow = r * cols * planes;
    dim destRow = r * destcols * planes;

    for (dim c = 0; c < cols; c++) {
      dim srcCol = srcRow + c * planes;
      dim destCol = destRow + c * planes;

      for (dim p = 0; p < planes; p++) {
        y[destCol + p] = x[srcCol + p];
      }
    }
  }

  subpixel* src = &y[0];
  size_t width = cols;
  size_t height = rows;

  IntegerMatrix locs = mipmapLocations(dx);
  for (int i = 0; i < locs.nrow(); i++) {
    size_t left = locs(i, 0);
    size_t top = locs(i, 1);

    if (left == 0)
      break;

    subpixel* dst = (&y[0]) + (top*destcols*planes) + left*planes;
    boxfilter(src, width, height, planes, destcols,
      dst, destcols);

    width = std::ceil(width/2.0);
    height = std::ceil(height/2.0);
    src = dst;

    /*
    for (int r = 0; r < locs(i, 3); r++) {
      for (int c = 0; c < locs(i, 2); c++) {
        for (int p = 0; p < planes; p++) {
          y[(top+r)*destcols*planes + (left+c)*planes + p] = i+1;
        }
      }
    }
    */
  }

  return y;
}


// You can include R code blocks in C++ files processed with sourceCpp
// (useful for testing and development). The R code will be automatically
// run after the compilation.
//

/*** R
x <- as.raw(1:(4*6*8))
dim(x) <- c(4, 6, 8)
y <- doMipmap(x, dim(x))
*/
