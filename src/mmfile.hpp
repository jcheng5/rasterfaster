#ifndef MMFILE_HPP
#define MMFILE_HPP

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

template<class T>
class MMFile {
  boost::interprocess::file_mapping fm_;
  boost::interprocess::mapped_region mr_;

  T* begin_;
  T* end_;

public:

  MMFile(const std::string& path, boost::interprocess::mode_t mode) {
    try {
      fm_ = boost::interprocess::file_mapping(path.c_str(), mode);
      mr_ = boost::interprocess::mapped_region(fm_, mode);
    } catch(boost::interprocess::interprocess_exception& e) {
      Rcpp::stop("Cannot read file %s", path) ;
    }

    begin_ = static_cast<T*>(mr_.get_address());
    end_ = begin_ + mr_.get_size() / sizeof(T);
  }

  T* begin() {
    return begin_;
  }

  T* end() {
    return end_;
  }

  void flush() {
    mr_.flush();
  }

};

#endif
