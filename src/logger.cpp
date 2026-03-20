#include "logger.h"

Logger::Logger(const std::string& filename, int num_shards)
    : num_shards(num_shards), file(filename) {
    for (int i = 0; i < num_shards; i++)
        shards.push_back(std::make_unique<Shard>());

    for (int i = 0; i < num_shards; i++)
        worker_threads.emplace_back(&Logger::worker, this, i);
}

void Logger::log(std::string msg) {
    int idx = next_shard.fetch_add(1) % num_shards;
    Shard& shard = *shards[idx];
    {
        std::lock_guard<std::mutex> lock(shard.mtx);
        shard.q.push(std::move(msg));
    }
    shard.cv.notify_one();
}

void Logger::worker(int shard_id) {
    Shard& shard = *shards[shard_id];
    std::vector<std::string> batch;
    batch.reserve(1024);

    while (true) {
        {
            std::unique_lock<std::mutex> lock(shard.mtx);
            shard.cv.wait(lock, [&]() { 
                return !shard.q.empty() || done.load(); 
            });

            while (!shard.q.empty()) {
                batch.push_back(std::move(shard.q.front()));
                shard.q.pop();
            }
        }

        {
            std::lock_guard<std::mutex> file_lock(file_mtx);
            for (auto& msg : batch)
                file << msg << "\n";
        }

        batch.clear();

        if (done) {
            // drain anything left after done was set
            std::unique_lock<std::mutex> lock(shard.mtx);
            while (!shard.q.empty()) {
                std::lock_guard<std::mutex> file_lock(file_mtx);
                file << shard.q.front() << "\n";
                shard.q.pop();
            }
            break;
        }
    }
}

Logger::~Logger() {
    done = true;
    for (auto& shard : shards)
        shard->cv.notify_one();
    for (auto& t : worker_threads)
        t.join();
    file.flush();
}
