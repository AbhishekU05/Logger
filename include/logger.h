#pragma once
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <vector>
#include <atomic>

class Logger {
public:
    Logger(const std::string& filename, int num_shards = 8);
    ~Logger();
    void log(std::string msg);

private:
    void worker(int shard_id);

    struct Shard {
        std::queue<std::string> q;
        std::mutex mtx;
        std::condition_variable cv;
    };

    std::vector<std::unique_ptr<Shard>> shards;
    int num_shards;
    std::atomic<int> next_shard = 0;
    std::atomic<bool> done = false;

    std::mutex file_mtx;
    std::ofstream file;

    std::vector<std::thread> worker_threads;
};
