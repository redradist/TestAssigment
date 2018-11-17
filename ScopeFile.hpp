#ifndef AUTOSAR_SCOPEFILE_HPP
#define AUTOSAR_SCOPEFILE_HPP

#include <iostream>
#include <fstream>

namespace Autosar {

class ScopeFile {
 public:
  ScopeFile(const std::string & _fileName,
               std::ios_base::openmode _mode = std::ios_base::out | std::ios_base::trunc) {
    file_.open(_fileName, _mode);
  }

  ~ScopeFile() {
    if (file_.is_open()) {
      file_.close();
    }
  }

  template <typename TMsg>
  friend ScopeFile & operator<<(ScopeFile & _scopeFile, const TMsg & _msg) {
    _scopeFile.file_ << _msg;
    return _scopeFile;
  }

  inline
  ScopeFile & operator<<(std::basic_ostream<char>&(*_manip)(std::basic_ostream<char>&)) {
    _manip(file_);
    return *this;
  }

 private:
  std::ofstream file_;
};



}

#endif  // AUTOSAR_SCOPEFILE_HPP
