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

    int N = 7;
    int K = 100000;
    double base_lambda = 10000.0;

    std::vector<std::thread> generators;
    std::vector<std::vector<long>> latencies(N);
    std::vector<std::vector<long>> build_latencies(N);

    for (int i = 0; i < N; i++) {
        latencies[i].reserve(K);
        build_latencies[i].reserve(K);
    }

    std::mt19937 main_rng(42);
    std::normal_distribution<double> lambda_dist(base_lambda, base_lambda * 0.2);
    std::vector<double> lambdas(N);
    for (int i = 0; i < N; i++)
        lambdas[i] = std::max(1000.0, lambda_dist(main_rng));

    printf("Mode       : %s\n", stress ? "STRESS (no sleep)" : "NORMAL (exponential inter-arrival)");
    printf("Generators : %d\n", N);
    printf("Events each: %d\n", K);
    if (!stress)
        printf("Base lambda: %.0f logs/sec\n", base_lambda);
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

                auto t_build_start = std::chrono::steady_clock::now();
                char buf[256];
                snprintf(buf, sizeof(buf), "[gen %d] [lambda %d] [t=%ld] event %d",
                         i, (int)lambdas[i], now, k);
                auto t_build_end = std::chrono::steady_clock::now();

                auto t0 = std::chrono::steady_clock::now();
                bool accepted = logger.log(buf);
                auto t1 = std::chrono::steady_clock::now();

                build_latencies[i].push_back(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(t_build_end - t_build_start).count()
                );

                if (accepted) {
                    latencies[i].push_back(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()
                    );
                }
            }
        });
    }

    for (auto& g : generators) g.join();

    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    long total = N * K;

    // flatten latencies
    std::vector<long> all_log, all_build;
    for (auto& v : latencies)
        for (auto x : v)
            all_log.push_back(x);
    for (auto& v : build_latencies)
        for (auto x : v)
            all_build.push_back(x);

    std::sort(all_log.begin(), all_log.end());
    std::sort(all_build.begin(), all_build.end());

    auto percentile = [](std::vector<long>& v, double p) {
        return v[static_cast<size_t>(p / 100.0 * v.size())];
    };

    printf("--- Per Generator ---\n");
    for (int i = 0; i < N; i++) {
        auto& v = latencies[i];
        std::sort(v.begin(), v.end());
        long drops = K - (long)v.size();
        printf("gen %d  lambda=%-8.0f  p50=%ld ns  p99=%ld ns  drops=%ld\n",
            i, lambdas[i],
            v[v.size() * 50 / 100],
            v[v.size() * 99 / 100],
            drops
        );
    }

    long total_accepted = 0;
    for (auto& v : latencies)
        total_accepted += v.size();


    printf("\n--- Overall ---\n");
    printf("Total logs : %ld\n", total);
    printf("Elapsed    : %.3f sec\n", elapsed);
    printf("Throughput (attempted) : %.0f logs/sec\n", (N * K) / elapsed);
    printf("Throughput (accepted)  : %.0f logs/sec\n", total_accepted / elapsed);
    printf("\n");
    printf("--- log() latency ---\n");
    printf("p50  : %ld ns\n", percentile(all_log, 50));
    printf("p99  : %ld ns\n", percentile(all_log, 99));
    printf("p999 : %ld ns\n", percentile(all_log, 99.9));
    printf("max  : %ld ns\n", all_log.back());
    printf("\n");
    printf("--- string build latency ---\n");
    printf("p50  : %ld ns\n", percentile(all_build, 50));
    printf("p99  : %ld ns\n", percentile(all_build, 99));
    printf("p999 : %ld ns\n", percentile(all_build, 99.9));
    printf("max  : %ld ns\n", all_build.back());

    return 0;
}
