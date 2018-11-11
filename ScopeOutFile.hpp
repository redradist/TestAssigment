#ifndef AUTOSAR_SCOPEFILE_HPP
#define AUTOSAR_SCOPEFILE_HPP

#include <iostream>
#include <fstream>

namespace Autosar {

class ScopeOutFile {
 public:
  ScopeOutFile(const std::string & _fileName,
               std::ios_base::openmode _mode = std::ios_base::out | std::ios_base::trunc) {
    file_.open(_fileName, _mode);
  }

  ~ScopeOutFile() {
    if (file_.is_open()) {
      file_.close();
    }
  }

  operator std::ofstream&() {
    return file_;
  }

 private:
  std::ofstream file_;
};

}

#endif  // AUTOSAR_SCOPEFILE_HPP
