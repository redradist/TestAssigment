#ifndef AUTOSAR_COMPONENT_HPP
#define AUTOSAR_COMPONENT_HPP

#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <boost/lockfree/queue.hpp>

#include "RandomBlockGenerator.hpp"

namespace Autosar {

class ActiveObject {
 public:
  ActiveObject(const size_t _queueSize)
    : queue{_queueSize} {
  }
  ~ActiveObject() {
  }

  void push(Autosar::RandomBlockInfo * _blockInfoPtr) {
    queue.push(_blockInfoPtr);
    queue_var_.notify_one();
  }

  void pop(Autosar::RandomBlockInfo *& _blockInfoPtr) {
    while (queue.empty() || !queue.pop(_blockInfoPtr)) {
      std::unique_lock<std::mutex> lock{mutex_};
      queue_var_.wait(lock, [this] {
        return !queue.empty();
      });
    }
    assert(_blockInfoPtr != nullptr);
  }

 private:
  std::mutex mutex_;
  std::condition_variable queue_var_;
  boost::lockfree::queue<Autosar::RandomBlockInfo *> queue;
};

}

#endif  // AUTOSAR_COMPONENT_HPP
