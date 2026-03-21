#include "logger.h"

Logger::Logger(const std::string& filename, size_t) : file(filename) {
    worker_thread = std::thread(&Logger::worker, this);
}

bool Logger::log(std::string msg) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(std::move(msg));
    }
    cv.notify_one();
    return true;
}

void Logger::worker() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return !q.empty() || done; });

        while (!q.empty()) {
            std::string msg = q.front();
            q.pop();
            lock.unlock();
            file << msg << "\n";
            lock.lock();
        }

        if (done) break;
    }
    file.flush();
}

Logger::~Logger() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        done = true;
    }
    cv.notify_one();
    worker_thread.join();
}
