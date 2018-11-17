//
// Created by redra on 17.11.18.
//

#ifndef UNTITLED1_LOGGER_HPP
#define UNTITLED1_LOGGER_HPP

#include <iostream>
#include <mutex>
#include "ILogger.hpp"
#include "ScopeFile.hpp"

namespace Autosar {

class Logger : public ILogger {
 public:
  Logger(const std::string & _msg)
    : log_file_{_msg,
                std::ios::out | std::ios::app | std::ios::ate} {
  }

  void error(const std::string & _msg) const override {
    std::lock_guard<std::mutex> lck{io_mutex_};
    log_file_ << _msg << std::endl;
    std::cerr << _msg << std::endl;
  }

 protected:
  mutable std::mutex io_mutex_;
  mutable Autosar::ScopeFile log_file_;
};

}

#endif  // UNTITLED1_LOGGER_HPP
