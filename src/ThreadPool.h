#pragma once

class ThreadPool {
public:
    explicit ThreadPool(unsigned short port, bool stopLog = false);

    ThreadPool(const ThreadPool &threadPool) = delete;

    ThreadPool(ThreadPool &&threadPool) = delete;
};
