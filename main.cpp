#include <iostream>
#include <list>
#include <atomic>
#include <thread>
#include <condition_variable>

#include <boost/program_options.hpp>
#include <boost/crc.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/format.hpp>
#include <mutex>

#include "RandomBlockGenerator.hpp"
#include "ActiveObject.hpp"
#include "Logger.hpp"

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

bool tryIncreaseListCount(std::atomic<unsigned> & _listCount, const unsigned & _limit) {
  unsigned listSize;
  bool succeed = false;
  do {
    listSize = _listCount.load(std::memory_order::memory_order_acquire);
  } while ((listSize + 1) <= _limit &&
           !(succeed = _listCount.compare_exchange_strong(listSize, listSize + 1, std::memory_order::memory_order_acq_rel)));
  return succeed;
}

bool tryDecreaseListCount(std::atomic<unsigned> & _listCount) {
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
      Autosar::Logger logger{"./out_log.txt"};
      std::atomic<unsigned> genCount{0};
      std::vector<std::unique_ptr<Autosar::ActiveObject>> objects;
      std::vector<std::thread> aThreads;
      std::vector<std::thread> bThreads;
      const auto & kRandomGenerator = Autosar::RandomBlockGenerator<unsigned>(kParams.blockSize, 0, 100);
      for (unsigned numB = 0; numB < kParams.numberOfBThreads; ++numB) {
        objects.push_back(std::move(std::make_unique<Autosar::ActiveObject>(kParams.maxNumBlock)));
      }
      for (unsigned numB = 0; numB < kParams.numberOfBThreads; ++numB) {
        bThreads.emplace_back([&, numB] {
          for (unsigned handCount = 0; handCount < kParams.maxNumBlock; ++handCount) {
            Autosar::RandomBlockInfo * blockInfoPtr;
            objects[numB]->pop(blockInfoPtr);
            const unsigned long kCheckSum = getCRC32(blockInfoPtr->block_, blockInfoPtr->block_size_);
            if (kCheckSum != blockInfoPtr->crc32_.load(std::memory_order::memory_order_acquire)) {
              blockInfoPtr->is_valid_.store(false, std::memory_order::memory_order_release);
            }
            if (not blockInfoPtr->isReclaimNeeded()) {
              blockInfoPtr->handled_times_.fetch_add(1, std::memory_order::memory_order_acq_rel);
            } else {
              blockInfoPtr->handled_times_.fetch_add(1, std::memory_order::memory_order_acq_rel);
              if (!blockInfoPtr->is_valid_.load(std::memory_order_acquire)) {
                const std::string kBlockMsg = blockInfoPtr->toString();
                logger.error((boost::format("CRC32 is not matched for block: {}") % kBlockMsg).str());
              }
              delete blockInfoPtr;
            }
          }
        });
      }
      for (unsigned i = 0; i < kParams.numberOfAThreads; ++i) {
        aThreads.emplace_back([&] {
          while (true) {
            if (!tryIncreaseListCount(genCount, kParams.maxNumBlock)) {
              break;
            } else {
              try {
                auto blockInfoPtr = new Autosar::RandomBlockInfo(kRandomGenerator.generateIntegerBlock(),
                                                                 kRandomGenerator.getBlockSize(),
                                                                 kParams.numberOfBThreads);
                blockInfoPtr->crc32_ = getCRC32(blockInfoPtr->block_, blockInfoPtr->block_size_);
                for (auto & object : objects) {
                  object->push(blockInfoPtr);
                }
              } catch (...) {
                tryDecreaseListCount(genCount);
                logger.error("Exception was caught !!");
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