#pragma once

#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>


class ThreadPool
{

public:

    // 创建线程数量
    explicit ThreadPool(size_t threadCount);


    // 析构，关闭线程
    ~ThreadPool();



    // 添加任务
    void AddTask(
        std::function<void()> task);



private:


    // 工作线程函数
    void Worker();



private:


    // 工作线程
    std::vector<std::thread> workers;



    // 任务队列
    std::queue<
        std::function<void()>
    > tasks;



    // 保护任务队列
    std::mutex mutex;



    // 唤醒线程
    std::condition_variable condition;



    // 是否停止
    bool stop;

};