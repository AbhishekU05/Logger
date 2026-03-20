#include "logger.h"
#include <thread>
#include <vector>
#include <chrono>
#include <cstdio>
#include <algorithm>
#include <atomic>
#include <string>
#include <random>
#include <cstring>

int main(int argc, char* argv[]) {
    bool stress = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--stress") == 0)
            stress = true;
    }

    Logger logger("out.log");

    int N = 8;
    int K = 100000;
    double base_lambda = 10000.0;

    std::vector<std::thread> generators;
    std::vector<std::vector<long>> latencies(N);

    for (int i = 0; i < N; i++)
        latencies[i].reserve(K);

    // sample per-generator lambdas
    std::mt19937 main_rng(42);
    std::normal_distribution<double> lambda_dist(base_lambda, base_lambda * 0.2);
    std::vector<double> lambdas(N);
    for (int i = 0; i < N; i++)
        lambdas[i] = std::max(1000.0, lambda_dist(main_rng));

    printf("Mode       : %s\n", stress ? "STRESS (no sleep)" : "NORMAL (exponential inter-arrival)");
    printf("Generators : %d\n", N);
    printf("Events each: %d\n", K);
    if (!stress) {
        printf("Base lambda: %.0f logs/sec\n", base_lambda);
    }
    printf("\n");

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < N; i++) {
        generators.emplace_back([&, i]() {
            std::mt19937 rng(i * 1000);
            std::exponential_distribution<double> inter_arrival(lambdas[i]);

            for (int k = 0; k < K; k++) {
                if (!stress) {
                    double wait_sec = inter_arrival(rng);
                    auto wait_ns = static_cast<long>(wait_sec * 1e9);
                    std::this_thread::sleep_for(std::chrono::nanoseconds(wait_ns));
                }

                auto now = std::chrono::steady_clock::now().time_since_epoch().count();
                std::string msg = "[gen " + std::to_string(i) +
                                  "] [lambda " + std::to_string((int)lambdas[i]) +
                                  "] [t=" + std::to_string(now) +
                                  "] event " + std::to_string(k);

                auto t0 = std::chrono::steady_clock::now();
                logger.log(std::move(msg));
                auto t1 = std::chrono::steady_clock::now();

                latencies[i].push_back(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()
                );
            }
        });
    }

    for (auto& g : generators) g.join();

    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    long total = N * K;

    std::vector<long> all;
    all.reserve(total);
    for (auto& v : latencies)
        for (auto x : v)
            all.push_back(x);

    std::sort(all.begin(), all.end());

    auto percentile = [&](double p) {
        return all[static_cast<size_t>(p / 100.0 * all.size())];
    };

    printf("--- Per Generator ---\n");
    for (int i = 0; i < N; i++) {
        auto& v = latencies[i];
        std::sort(v.begin(), v.end());
        printf("gen %d  lambda=%-8.0f  p50=%ld ns  p99=%ld ns\n",
            i,
            lambdas[i],
            v[v.size() * 50 / 100],
            v[v.size() * 99 / 100]
        );
    }

    printf("\n--- Overall ---\n");
    printf("Total logs : %ld\n", total);
    printf("Elapsed    : %.3f sec\n", elapsed);
    printf("Throughput : %.0f logs/sec\n", total / elapsed);
    printf("Latency p50: %ld ns\n", percentile(50));
    printf("Latency p99: %ld ns\n", percentile(99));
    printf("Latency p999: %ld ns\n", percentile(99.9));
    printf("Latency max: %ld ns\n", all.back());

    return 0;
}
