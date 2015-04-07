#ifndef GRID_HPP
#define GRID_HPP

#include <algorithm>
#include <Rcpp.h>

// The type of the integer used for row/col numbers.
typedef size_t index_t;

// Grid is used to model a 2D matrix or strided array.
template <class T>
class Grid {
  T* _begin;
  const index_t _stride;
  const index_t _nrow;
  const index_t _ncol;

public:
  Grid(T* begin, T* end, index_t stride, index_t rows, index_t cols) :
    _begin(begin), _stride(stride), _nrow(rows), _ncol(cols) {

    if (end - begin != (rows * stride)) {
      Rcpp::warning("%d != %d", end-begin, rows*cols);
    }

    if (rows == 0 || cols == 0) {
      Rcpp::stop("Grid can't be created with 0 cells");
    }
  }

  T* at(index_t row, index_t col) const {
    row = std::min(std::max<index_t>(row, 0), _nrow-1);
    col = std::min(std::max<index_t>(col, 0), _ncol-1);
    return _begin + (row * _stride) + col;
  }

  const index_t nrow() const {
    return _nrow;
  }

  const index_t ncol() const {
    return _ncol;
  }
};

template <class T>
class GridIterator {
  const Grid<T>* _grid;
  const index_t _startCol;
  const index_t _endCol;
  const index_t _startRow;
  const index_t _endRow;
  index_t _col;
  index_t _row;

public:
  GridIterator(const Grid<T>* grid, index_t startCol, index_t endCol,
    index_t startRow, index_t endRow) : _grid(grid), _startCol(startCol),
    _endCol(endCol), _startRow(startRow), _endRow(endRow), _col(startCol),
    _row(startRow) {
  }
  ~GridIterator() {}

  void moveToEnd() {
    _col = _endCol;
    _row = _endRow;
  }

  bool operator==(const GridIterator& other) {
    return _grid == other._grid && _col == other._col && _row == other._row;
  }

  GridIterator& operator++() {
    _col++;
    if (_col == _endCol) {
      _row++;
      _col = _startCol;
    }
  }

  GridIterator& operator++(int) {
    GridIterator temp = *this;
    ++*this;
    return temp;
  }

  T& operator*() {
    return *_grid->at(_row, _col);
  }
};

#endif
