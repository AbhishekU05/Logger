#include "logger.h"
#include <iostream>

Logger::Logger(const std::string& filename, size_t qsize)
    : queue(qsize), file(filename), running(true) {
    worker_thread = std::thread(&Logger::worker, this);
}

Logger::~Logger() {
    running.store(false);
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

void Logger::log(const std::string& msg) {
    // DROP if full (important design decision)
    queue.push(msg);
}

void Logger::worker() {
    while (running.load()) {
        auto item = queue.pop();
        if (item) {
            file << *item << "\n";
        }
    }

    // flush remaining
    while (true) {
        auto item = queue.pop();
        if (!item) break;
        file << *item << "\n";
    }

    file.flush();
}
