#pragma once
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>

class Logger {
public:
    Logger(const std::string& filename);
    ~Logger();

    void log(const std::string& msg);

private:
    void worker();

    std::queue<std::string> q;

    std::mutex mtx;
    std::condition_variable cv;

    bool done = false;

    std::thread worker_thread;
    std::ofstream file;
};
