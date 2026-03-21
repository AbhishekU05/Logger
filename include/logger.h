#pragma once
#include <string>

#ifdef USE_LOCKFREE

#include "queue.h"
#include <thread>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>

class Logger {
public:
    Logger(const std::string& filename, size_t queue_capacity = 1024 * 256);
    ~Logger();
    bool log(std::string msg);

private:
    void worker();

    MPMCQueue<std::string> q;
    std::atomic<bool> done = false;
    int fd = -1;
    std::thread worker_thread;
};

#else

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>

class Logger {
public:
    Logger(const std::string& filename, size_t queue_capacity = 0);
    ~Logger();
    bool log(std::string msg);

private:
    void worker();

    std::queue<std::string> q;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    std::thread worker_thread;
    std::ofstream file;
};

#endif
