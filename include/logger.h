#pragma once
#include "queue.h"
#include <thread>
#include <fstream>
#include <atomic>

class Logger {
public:
    Logger(const std::string& filename, size_t queue_size = 1024);
    ~Logger();

    void log(const std::string& msg);

private:
    void worker();

    MPMCQueue<std::string> queue;
    std::thread worker_thread;
    std::ofstream file;

    std::atomic<bool> running;
};
