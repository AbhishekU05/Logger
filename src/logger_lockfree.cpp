#include "logger.h"
#include <sched.h>
#include <cstdio>

Logger::Logger(const std::string& filename, size_t queue_capacity)
    : q(queue_capacity) {
    fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        perror("failed to open log file");

    worker_thread = std::thread(&Logger::worker, this);
}

bool Logger::log(std::string msg) {
    return q.push(std::move(msg));
}

void Logger::worker() {
    constexpr int SPIN_COUNT = 10;
    std::vector<std::string> batch;
    std::string out;
    batch.reserve(1024);
    out.reserve(1024 * 128); // 128KB output buffer

    while (true) {
        // spin phase
        int spins = 0;
        while (spins < SPIN_COUNT) {
            auto msg = q.pop();
            if (msg) {
                batch.push_back(std::move(*msg));
                while (auto m = q.pop())
                    batch.push_back(std::move(*m));
                break;
            }
            spins++;
        }

        if (!batch.empty()) {
            // build one contiguous buffer
            out.clear();
            for (auto& m : batch)  {
                out += m;
                out += '\n';
            }

            // one syscall for entire batch
            //auto w0 = std::chrono::steady_clock::now();
            ::write(fd, out.data(), out.size());
            /*
            auto w1 = std::chrono::steady_clock::now();
            auto write_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(w1 - w0).count();
            if (write_ns > 100000)
                printf("slow write: %ld ns for %zu messages\n", write_ns, batch.size());

            static int drain_count = 0;
            if (++drain_count % 1000 == 0)
                printf("batch size: %zu\n", batch.size());
            */
            batch.clear();
            continue;
        }

        if (done) {
            // final drain
            while (auto msg = q.pop()) {
                out += *msg;
                out += '\n';
            }
            if (!out.empty())
                ::write(fd, out.data(), out.size());
            break;
        }

        sched_yield();
    }
}

Logger::~Logger() {
    done = true;
    worker_thread.join();
    close(fd);
}
