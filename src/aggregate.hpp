#ifndef AGGREGATE_HPP
#define AGGREGATE_HPP

#include <stdlib.h>
#include <limits>
#include <vector>

template <class T>
double mean(T* begin, T* end) {
  long double result = 0;
  size_t length = end - begin;

  for (; begin != end; begin++) {
    result += static_cast<long double>(*begin) / length;
  }

  return result;
}

template <class T>
bool mode(std::vector<T>& vec, T* result) {
  // Sort the values
  std::sort(vec.begin(), vec.end());

  // Now walk the sorted vector, looking for the longest run of equal values.
  // If we have ties, pick a winner at random.

  typename std::vector<T>::const_iterator prev = vec.begin(),
    pos = vec.begin();
  pos++;

  // Number of consecutive times we've seen the value of *prev
  size_t currCount = 1;
  // Maximum value of currCount
  size_t maxCount = 0;
  // Number of times we've tied
  size_t ties = 0;

  for (; pos != vec.end(); pos++, prev++, currCount++) {
    if (*pos != *prev) {
      // The value changed. Is it tied with the winner?
      bool winner = false;
      if (currCount == maxCount) {
        ties++;
        // With each additional tie (at the same count), the chance of the new
        // value being the winner should go down, so that overall the chances
        // are equal for any winner at the same count.
        if (::rand() % (ties + 1) == 0) {
          winner = true;
        }
      } else if (currCount > maxCount) {
        // We've exceeded all past winners. Not only are we the winner, there's
        // also never been a tie with this count.
        winner = true;
        ties = 0;
      }

      // The *prev value is the winner; set the maxCount, then reset the
      // currCount for the next value.
      if (winner) {
        maxCount = currCount;
        *result = *prev;
      }

      // Regardless of whether we were the winner or not, reset the count.
      currCount = 0;
    }
  }

  if (currCount == maxCount) {
    ties++;
    if (::rand() % (ties + 1) == 0) {
      *result = *prev;
    }
  } else if (currCount > maxCount) {
    *result = *prev;
  }

  return true;
}

template <class T, class TIter>
bool mode(TIter begin, TIter end, T* result) {
  if (begin == end) {
    // Can't take the mode of a zero values
    return false;
  }

  // Copy the values into a vector we can mutate
  std::vector<T> vec(begin, end);
  return mode<T>(vec, result);
}

#endif
