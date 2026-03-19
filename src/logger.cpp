#include "logger.h"

Logger::Logger(const std::string& filename) : file(filename) {
    worker_thread = std::thread(&Logger::worker, this);
}

void Logger::log(const std::string& msg) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(msg);
    }
    cv.notify_one();
}

void Logger::worker() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [&]() {
            return !q.empty() || done;
        });

        while (!q.empty()) {
            std::string msg = q.front();
            q.pop();

            lock.unlock();              // don't hold lock during I/O
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
