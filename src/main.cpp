#include "logger.h"
#include <thread>
#include <vector>
#include <chrono>
#include <cstdio>
#include <algorithm>
#include <string>

int main() {
    Logger logger("out.log");

    int threads = 8;
    int logs_per_thread = 10000;

    std::vector<std::thread> workers;
    std::vector<std::vector<long>> latencies(threads);

    for (int t = 0; t < threads; t++) {
        latencies[t].reserve(logs_per_thread);
    }

    auto start = std::chrono::steady_clock::now();

    for (int t = 0; t < threads; t++) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < logs_per_thread; i++) {
                auto now = std::chrono::steady_clock::now().time_since_epoch().count();
                std::string msg = "[thread " + std::to_string(t) + "] [" +
                                  std::to_string(now) + "] event number " +
                                  std::to_string(i);

                auto t0 = std::chrono::steady_clock::now();
                logger.log(std::move(msg));
                auto t1 = std::chrono::steady_clock::now();
                latencies[t].push_back(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()
                );
            }
        });
    }

    for (auto& t : workers) t.join();

    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    long total = threads * logs_per_thread;

    std::vector<long> all;
    all.reserve(total);
    for (auto& v : latencies)
        for (auto x : v)
            all.push_back(x);

    std::sort(all.begin(), all.end());

    auto percentile = [&](double p) {
        return all[static_cast<size_t>(p / 100.0 * all.size())];
    };

    printf("Throughput : %.0f logs/sec\n", total / elapsed);
    printf("Total time : %.3f sec\n", elapsed);
    printf("Latency p50: %ld ns\n", percentile(50));
    printf("Latency p99: %ld ns\n", percentile(99));
    printf("Latency p999: %ld ns\n", percentile(99.9));
    printf("Latency max: %ld ns\n", all.back());

    return 0;
}
