#include "logger.h"
#include <thread>
#include <vector>

int main() {
    Logger logger("out.log");

    int threads = 4;
    int logs_per_thread = 10000;

    std::vector<std::thread> workers;

    for (int t = 0; t < threads; t++) {
        workers.emplace_back([&]() {
            for (int i = 0; i < logs_per_thread; i++) {
                logger.log("hello");
            }
        });
    }

    for (auto& t : workers) {
        t.join();
    }

    return 0;
}
