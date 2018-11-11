#ifndef AUTOSAR_LOCKFREELIST_HPP
#define AUTOSAR_LOCKFREELIST_HPP

#include <list>
#include <atomic>

namespace Autosar {

template <typename TItem>
class LockFreeList {
 public:
  class NodeIterator {
   public:
    NodeIterator(typename LockFreeList::Node * _node)
        : node_{_node} {
    }

    void operator++() {
      if (node_ != nullptr) {
        node_ = node_->next_.load();
      }
    }

    void operator++(int) {
      this->operator++();
    }

    TItem & operator*() {
      return node_->item_;
    }

    bool operator ==(const NodeIterator & _iter) {
      return node_ == _iter.node_;
    }

    bool operator !=(const NodeIterator & _iter) {
      return !(this->operator==(_iter));
    }

   private:
    typename LockFreeList::Node * node_;
  };

  typedef TItem & reference;
  typedef const TItem & const_reference;
  typedef LockFreeList::NodeIterator iterator;
  typedef const LockFreeList::NodeIterator const_iterator;

  LockFreeList() = default;
  ~LockFreeList() = default;

  iterator begin() {
    return NodeIterator{front_.load()};
  }

  const_reference begin() const {
    return NodeIterator{front_.load()};
  }

  iterator end() {
    return NodeIterator{nullptr};
  }

  const_reference end() const {
    return NodeIterator{nullptr};
  }

  bool isEmpty() const {
    return nullptr == front_.load();
  }

  void pushBack(const TItem & _item) {
    Node * node = new Node(_item);
    Node * last = getLast();
    Node * dummy = nullptr;
    while (!last->next_.compare_exchange_strong(dummy, node)) {
      last = dummy;
    }
  }

  void pushBack(TItem && _item) {
    Node * node = new Node(std::move(_item));
    Node * dummy = nullptr;
    if (!front_.compare_exchange_strong(dummy, node)) {
      Node * last = getLast();
      dummy = nullptr;
      while (!last->next_.compare_exchange_strong(dummy, node)) {
        last = dummy;
        dummy = nullptr;
      }
    }
  }

 protected:
  struct Node {
    TItem item_;
    std::atomic<Node *> next_{nullptr};

    Node(const TItem & _item)
      : item_{_item} {
    }

    Node(TItem && _item)
        : item_{std::move(_item)} {
    }
  };

  Node * getLast() {
    Node * last_ = front_.load();
    while (last_ != nullptr) {
      Node * next = last_->next_.load();
      if (next == nullptr) {
        break;
      } else {
        last_ = next;
      }
    }
    return last_;
  }

 private:
  std::atomic<Node *> front_{nullptr};
};

}

#endif  // AUTOSAR_LOCKFREELIST_HPP
