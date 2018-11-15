#ifndef AUTOSAR_RANDOM_BLOCK_GENERATOR_HPP
#define AUTOSAR_RANDOM_BLOCK_GENERATOR_HPP

#include <random>
#include <memory>
#include <atomic>

namespace Autosar {

struct RandomBlockInfo {
  std::unique_ptr<unsigned[]> block_;
  const unsigned block_size_;
  std::atomic<unsigned long> crc32_{0};
  std::atomic<unsigned> handled_times_{0};
  const unsigned limit_handled_times_;
  std::atomic<bool> is_valid_{true};

  RandomBlockInfo(std::unique_ptr<unsigned[]> _block,
                  const unsigned _blockSize,
                  const unsigned _handledTimes)
      : block_{std::move(_block)}
      , block_size_{_blockSize}
      , limit_handled_times_{_handledTimes} {
  }
  RandomBlockInfo(RandomBlockInfo&& _blockInfo)
      : block_{std::move(_blockInfo.block_)}
      , block_size_{_blockInfo.block_size_}
      , limit_handled_times_{_blockInfo.limit_handled_times_} {
    handled_times_.store(_blockInfo.handled_times_);
    crc32_.store(_blockInfo.crc32_);
    is_valid_.store(_blockInfo.is_valid_);
  }

  bool isReclaimNeeded() const {
    return 0 == limit_handled_times_ ||
           (limit_handled_times_ - 1) <= handled_times_.load(std::memory_order::memory_order_acquire);
  }
};

template <typename TRandomNumberType>
class RandomBlockGenerator {
 public:
  RandomBlockGenerator(const unsigned _size,
                       const TRandomNumberType _lowerBound,
                       const TRandomNumberType _upperBound)
      : size_{_size}
      , lower_bound_{_lowerBound}
      , upper_bound_{_upperBound} {
  }

  std::unique_ptr<TRandomNumberType[]> generateIntegerBlock() const {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<TRandomNumberType> dist(lower_bound_, upper_bound_);
    auto randBlock = std::make_unique<TRandomNumberType[]>(size_);
      for (unsigned i = 0; i < size_; ++i) {
      randBlock[i] = dist(mt);
    }
    return randBlock;
  }

  std::unique_ptr<TRandomNumberType[]> generateFloatBlock() const {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<TRandomNumberType> dist(lower_bound_, upper_bound_);
    auto randBlock = std::make_unique<TRandomNumberType[]>(new TRandomNumberType[size_]);
    for (unsigned i = 0; i < size_; ++i) {
      randBlock[i] = dist(mt);
    }
    return randBlock;
  }

  unsigned getBlockSize() const {
    return size_;
  }

 private:
  const unsigned size_;
  const TRandomNumberType lower_bound_;
  const TRandomNumberType upper_bound_;
};

}

#endif  // AUTOSAR_RANDOM_BLOCK_GENERATOR_HPP
