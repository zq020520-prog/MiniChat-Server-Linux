#include "ThreadPool.h"



ThreadPool::ThreadPool(size_t threadCount)
{

    stop = false;



    for (size_t i = 0;i < threadCount;i++)
    {


        workers.emplace_back(
            &ThreadPool::Worker,
            this
        );


    }

}




void ThreadPool::Worker()
{

    while (true)
    {


        std::function<void()> task;



        {

            std::unique_lock<std::mutex>
                lock(mutex);



            condition.wait(
                lock,
                [this]()
                {

                    return stop ||
                        !tasks.empty();

                }
            );



            // 退出条件

            if (stop && tasks.empty())
            {
                return;
            }



            task =
                tasks.front();


            tasks.pop();


        }



        // 执行任务

        task();


    }

}





void ThreadPool::AddTask(
    std::function<void()> task)
{

    {


        std::lock_guard<std::mutex>
            lock(mutex);



        tasks.push(task);


    }



    // 唤醒一个等待线程

    condition.notify_one();

}




ThreadPool::~ThreadPool()
{


    {


        std::lock_guard<std::mutex>
            lock(mutex);



        stop = true;

    }



    // 唤醒所有线程退出

    condition.notify_all();



    for (auto& worker : workers)
    {

        if (worker.joinable())
        {
            worker.join();
        }

    }


}