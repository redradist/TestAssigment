#ifndef AUTOSAR_LOCKLIST_HPP
#define AUTOSAR_LOCKLIST_HPP

#include <list>
#include <mutex>

namespace Autosar {

template <typename TItem>
class LockList {
 public:
  typedef typename std::list<TItem>::reference reference;
  typedef typename std::list<TItem>::const_reference const_reference;
  typedef typename std::list<TItem>::iterator iterator;
  typedef typename std::list<TItem>::const_iterator const_iterator;

  LockList() = default;
  LockList(std::initializer_list<TItem> _items)
    : list_{_items} {
  }
  ~LockList() = default;

  iterator begin() {
    return list_.begin();
  }

  const_reference begin() const {
    return list_.begin();
  }

  iterator end() {
    return list_.end();
  }

  const_reference end() const {
    return list_.end();
  }

  bool isEmpty() const {
    return list_.empty();
  }

  void pushBack(const TItem & _item) {
    std::lock_guard<std::mutex> lock{mtx_};
    list_.push_back(_item);
  }

  void pushBack(TItem && _item) {
    std::lock_guard<std::mutex> lock{mtx_};
    list_.push_back(std::move(_item));
  }

  void popFront() {
    std::lock_guard<std::mutex> lock{mtx_};
    list_.pop_front();
  }

 private:
  std::mutex mtx_;
  std::list<TItem> list_;
};

}

#endif  // AUTOSAR_LOCKLIST_HPP
