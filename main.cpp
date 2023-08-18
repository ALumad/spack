#include <algorithm>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <random>
#include <bitset>

namespace Setting
{
    const size_t COUNT = 1000000;
    const int MIN = 1;
    const int MAX = 100;
    const size_t MAGIC_NUMBER = 7; //numbers[1,100] take no more than 7 bits
    const size_t BIT_TOTAL = MAGIC_NUMBER*COUNT;
    const size_t ALLOCATED_SIZE = BIT_TOTAL / 8 + (BIT_TOTAL % 8 > 0); //just for check

    const size_t core_numbers = std::thread::hardware_concurrency();
    const size_t number_per_task = COUNT/core_numbers;
    const size_t number_for_last_task = number_per_task + COUNT%core_numbers;
}

typedef std::vector<std::bitset<Setting::BIT_TOTAL> > HEAP_BITSET;

namespace Packing {
    void Pack(const int* inarr, std::bitset<Setting::BIT_TOTAL>& outarr, const std::size_t offset, const std::size_t size){
        for (std::size_t i = offset; i < offset+size; ++i){
            for (std::size_t b = 0; b < Setting::MAGIC_NUMBER; ++b){
                outarr.set(Setting::MAGIC_NUMBER*i + b,(inarr[i] & ( 1 << b )) >> b);
            }
        }
    }

    void Unpack(std::bitset<Setting::BIT_TOTAL>& inarr, int* outarr, const std::size_t offset, const std::size_t size){
        for (std::size_t i = offset; i < size+offset; ++i){
            for (std::size_t b = 0; b < Setting::MAGIC_NUMBER; ++b){
                outarr[i] |= inarr.test(Setting::MAGIC_NUMBER*i + b) << b;
            }
        }
    }

}


namespace Multithreading {
    void Pack(const int* inarr, std::bitset<Setting::BIT_TOTAL>& outarr) {
        std::vector<std::thread> tasks;
        for (size_t i = 0; i < Setting::core_numbers; ++i){
            int numbers = (i + 1 < Setting::core_numbers ? Setting::number_per_task : Setting::number_for_last_task);
            tasks.emplace_back([&, o=Setting::number_per_task*i, n=numbers]{Packing::Pack(inarr, outarr, o, n);});
        }

        for (size_t i = 0; i < Setting::core_numbers; ++i){
            tasks[i].join();
        }
    }

    void Unpack(std::bitset<Setting::BIT_TOTAL>& inarr, int* outarr) {
        std::vector<std::thread> tasks;
        for (size_t i = 0; i < Setting::core_numbers; ++i){
            int numbers = (i + 1 < Setting::core_numbers ? Setting::number_per_task : Setting::number_for_last_task);
            tasks.emplace_back([&, o=Setting::number_per_task*i, n=numbers]{Packing::Unpack(inarr, outarr, o, n);});
        }

        for (size_t i = 0; i < Setting::core_numbers; ++i){
            tasks[i].join();
        }

    }
}

int main(){
    using namespace Setting;

    std::vector<int> seq;
    seq.reserve(COUNT);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(MIN, MAX);
    for (int i=0; i <COUNT; ++i){
        seq.push_back(distrib(gen));
    }

    std::vector<int> seq_unpack(COUNT,0);
    HEAP_BITSET heap_bitset(1,0);

    /*It equals on MacOs i9 clang++,  and Win10 CygWin i5 g++*/
    if (ALLOCATED_SIZE != sizeof(heap_bitset[0])) {
        /*I guess it might be not equal for some systems*/
        std::cout << "There is a difference between theoretical and actual memory usage\n"
                  << "Theoretical: " << ALLOCATED_SIZE << std::endl
                  << "Actual: " << sizeof(heap_bitset[0]) << std::endl;

    }

    Multithreading::Pack(seq.data(), heap_bitset[0]);
    std::cout << "Packing completed\n\n";

    std::cout << "Original size: " << seq.size()*sizeof(int) << std::endl
              << "Packed size: " << sizeof(heap_bitset[0]) << std::endl;

    Multithreading::Unpack(heap_bitset[0],seq_unpack.data());
    std::cout << "\nUnpacking completed\n";

    std::cout << "\nChecking result\n";
    for (int i=0; i < COUNT; ++i){
        if (seq_unpack[i] != seq[i]) {
            std::cout << i << " " << seq_unpack[i] << " " << seq[i] << std::endl;
            return 1;
        }
    }
    std::cout << "Success\n";

    return 0;
}