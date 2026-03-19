#include "logger.h"
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>

int main() {
    Logger logger("bench.log", 1 << 16);

    int threads = 8;
    int logs_per_thread = 100000;

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> workers;

    for (int t = 0; t < threads; t++) {
        workers.emplace_back([&]() {
            for (int i = 0; i < logs_per_thread; i++) {
                logger.log("msg");
            }
        });
    }

    for (auto& th : workers) th.join();

    auto end = std::chrono::high_resolution_clock::now();

    double seconds =
        std::chrono::duration<double>(end - start).count();

    std::cout << "Throughput: "
              << (threads * logs_per_thread) / seconds
              << " logs/sec\n";
}
