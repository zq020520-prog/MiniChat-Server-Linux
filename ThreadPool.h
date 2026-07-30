#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>


class ThreadPool
{

public:

    ThreadPool(int size);

    ~ThreadPool();


    void Submit(
        std::function<void()> task);


private:

    std::vector<std::thread> workers;


    std::queue<std::function<void()>> tasks;


    std::mutex mtx;

    std::condition_variable cv;


    bool stop;


};