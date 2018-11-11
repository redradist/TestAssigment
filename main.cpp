#include <iostream>
#include <list>
#include <atomic>
#include <thread>
#include <condition_variable>

#include <boost/program_options.hpp>
#include <boost/crc.hpp>
#include <mutex>

#include "RandomBlockGenerator.hpp"
#include "ScopeOutFile.hpp"

struct ProgramParams {
  unsigned numberOfAThreads = 0;
  unsigned numberOfBThreads = 0;
  unsigned maxNumBlock = 0;
  unsigned blockSize = 0;
};

ProgramParams parseArguments(const boost::program_options::variables_map & _args) {
  ProgramParams params;
  if (_args.count("numA")) {
    std::cout << "Number of А threads: " << _args["numA"].as<unsigned>() << std::endl;
    params.numberOfAThreads = _args["numA"].as<unsigned>();
  } else if (_args.count("a")) {
    std::cout << "Number of А threads: " << _args["a"].as<unsigned>() << std::endl;
    params.numberOfAThreads = _args["a"].as<unsigned>();
  }
  if (_args.count("numB")) {
    std::cout << "Number of B threads: " << _args["numB"].as<unsigned>() << std::endl;
    params.numberOfBThreads = _args["numB"].as<unsigned>();
  } else if (_args.count("b")) {
    std::cout << "Number of B threads: " << _args["b"].as<unsigned>() << std::endl;
    params.numberOfBThreads = _args["b"].as<unsigned>();
  }
  if (_args.count("maxNumBlock")) {
    std::cout << "Number of block: " << _args["maxNumBlock"].as<unsigned>() << std::endl;
    params.maxNumBlock = _args["maxNumBlock"].as<unsigned>();
  } else if (_args.count("n")) {
    std::cout << "Number of block: " << _args["n"].as<unsigned>() << std::endl;
    params.maxNumBlock = _args["n"].as<unsigned>();
  }
  if (_args.count("blockSize")) {
    std::cout << "Number of B threads: " << _args["blockSize"].as<unsigned>() << std::endl;
    params.blockSize = _args["blockSize"].as<unsigned>();
  } else if (_args.count("s")) {
    std::cout << "Number of B threads: " << _args["s"].as<unsigned>() << std::endl;
    params.blockSize = _args["s"].as<unsigned>();
  }
  return params;
}

bool increaseListCount(std::atomic<unsigned> & _listCount, const unsigned & _limit) {
  unsigned listSize;
  bool succeed = false;
  do {
    listSize = _listCount.load(std::memory_order::memory_order_acquire);
  } while ((listSize + 1) < _limit &&
           !(succeed = _listCount.compare_exchange_strong(listSize, listSize + 1, std::memory_order::memory_order_acq_rel)));
  return succeed;
}

bool decreaseListCount(std::atomic<unsigned> & _listCount) {
  unsigned listSize;
  bool succeed = false;
  do {
    listSize = _listCount.load(std::memory_order::memory_order_acquire);
  }
  while (listSize > 0 &&
         !(succeed = _listCount.compare_exchange_strong(listSize, listSize - 1, std::memory_order::memory_order_acq_rel)));
  return succeed;
}

unsigned long getCRC32(std::unique_ptr<unsigned[]> & _block,
                       const unsigned _blockSize) {
  boost::crc_32_type result;
  result.process_bytes(_block.get(), _blockSize);
  return result.checksum();
}

int main(int argc, const char *argv[]) {
  try {
    boost::program_options::options_description desc{"Options"};
    desc.add_options()
        ("help,h", "Help screen. This version of the program generates infinite number of blocks")
        ("numA,a", boost::program_options::value<unsigned>()->default_value(1), "Number of А threads")
        ("numB,b", boost::program_options::value<unsigned>()->default_value(1), "Number of B threads")
        ("maxNumBlock,n", boost::program_options::value<unsigned>(), "Number of block")
        ("blockSize,s", boost::program_options::value<unsigned>(), "Size of block");

    boost::program_options::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
    } else {
      const ProgramParams kParams = parseArguments(vm);
      Autosar::ScopeOutFile logFile{"./out_log.txt",
                                    std::ios::out | std::ios::app | std::ios::ate};
      std::recursive_mutex ioMutex;
      std::atomic<unsigned> genCount{0};
      std::list<Autosar::RandomBlockInfo> blocks;
      std::vector<std::thread> aThreads;
      std::vector<std::thread> bThreads;
      const auto & kRandomGenerator = Autosar::RandomBlockGenerator<unsigned>(kParams.blockSize, 0, 100);
      for (unsigned i = 0; i < kParams.numberOfAThreads; ++i) {
        aThreads.emplace_back([&] {
          while (true) {
            if (!increaseListCount(genCount, kParams.maxNumBlock)) {
              break;
            } else {
              try {
                Autosar::RandomBlockInfo blockInfo{kRandomGenerator.generateIntegerBlock(),
                                                   kRandomGenerator.getBlockSize()};
                blockInfo.crc32_ = getCRC32(blockInfo.block_, blockInfo.block_size_);
                blocks.push_back(std::move(blockInfo));
              } catch (...) {
                decreaseListCount(genCount);
                std::lock_guard<std::recursive_mutex> lck{ioMutex};
                std::cerr << "Exception was caught !!" << std::endl;
              }
            }
          }
        });
      }
      for (unsigned i = 0; i < kParams.numberOfBThreads; ++i) {
        bThreads.emplace_back([&] {
          std::atomic<unsigned> handCount{0};
          bool hasNextItem = !blocks.empty();
          auto frontItem = blocks.begin();
          while (true) {
            if (!increaseListCount(handCount, kParams.maxNumBlock)) {
              break;
            } else {
              if (blocks.empty()) {
                hasNextItem = false;
              } else if (hasNextItem) {
                Autosar::RandomBlockInfo & blockInfo = *frontItem;
                unsigned long checkSum = getCRC32(blockInfo.block_, blockInfo.block_size_);
                if (checkSum != blockInfo.crc32_.load(std::memory_order::memory_order_acquire)) {
                  blockInfo.is_valid_.store(false, std::memory_order::memory_order_release);
                }
                blockInfo.handled_times_.fetch_add(1, std::memory_order::memory_order_acq_rel);
                frontItem++;
                hasNextItem = frontItem != blocks.end();
                if (kParams.numberOfBThreads == blockInfo.handled_times_.load(std::memory_order::memory_order_acquire)) {
                  if (!blockInfo.is_valid_.load(std::memory_order_acquire)) {
                    std::stringstream msg;
                    msg << "CRC32 is not matched for block:" << std::endl;
                    msg << "  ";
                    for (unsigned i = 0; i < blockInfo.block_size_; ++i) {
                      msg << std::hex << blockInfo.block_[i];
                    }
                    msg << std::endl;
                    std::lock_guard<std::recursive_mutex> lck{ioMutex};
                    static_cast<std::ofstream&>(logFile) << msg.str();
                    std::cerr << msg.str();
                  }
                  blocks.pop_front();
                }
              }
            }
          }
        });
      }

      for (auto & thread : aThreads) {
        thread.join();
      }
      for (auto & thread : bThreads) {
        thread.join();
      }
    }
  } catch (const boost::program_options::error &ex) {
    std::cerr << ex.what() << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
  }
}