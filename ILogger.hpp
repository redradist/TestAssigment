//
// Created by redra on 17.11.18.
//

#ifndef UNTITLED1_ILOGGER_HPP
#define UNTITLED1_ILOGGER_HPP

#include <string>

class ILogger {
 public:
//  virtual void info() const = 0;
//  virtual void debug() const = 0;
//  virtual void warning() const = 0;
  virtual void error(const std::string & _msg) const = 0;
//  virtual void fatal() const = 0;
};

#endif //UNTITLED1_ILOGGER_HPP
