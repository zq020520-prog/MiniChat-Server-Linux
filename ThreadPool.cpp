#include "ThreadPool.h"


ThreadPool::ThreadPool(int size)
    :
    stop(false)
{


    for (int i = 0;i < size;i++)
    {

        workers.emplace_back(
            [this]()
            {

                while (true)
                {

                    std::function<void()> task;


                    {
                        std::unique_lock<std::mutex> lock(mtx);


                        cv.wait(lock,
                            [this]()
                            {
                                return stop || !tasks.empty();
                            });


                        if (stop && tasks.empty())
                            return;



                        task = tasks.front();

                        tasks.pop();

                    }


                    // 执行任务
                    task();

                }


            });


    }


}



void ThreadPool::Submit(
    std::function<void()> task)
{


    {
        std::lock_guard<std::mutex> lock(mtx);

        tasks.push(task);
    }


    cv.notify_one();

}



ThreadPool::~ThreadPool()
{


    {
        std::lock_guard<std::mutex> lock(mtx);

        stop = true;

    }


    cv.notify_all();


    for (auto& t : workers)
    {
        t.join();
    }

}