#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <iostream>

#include "thread_safe_queue.h"
#include "types.h"

namespace mini_ros
{
    class Executor
    {
    public:
        static Executor &instance()
        {
            static Executor executor;

            return executor;
        }

        /*
         * Start the thread pool with the specified number of threads.
         */
        void start(size_t threadCount = 10)
        {
            if (threadCount == 0)
            {
                threadCount = 4;
            }

            running_ = true;

            for (size_t i = 0; i < threadCount; ++i)
            {
                workers_.emplace_back([this, i]()
                                      { workerLoop(i); });
            }

            std::cout << "[Executor] Started with "
                      << threadCount
                      << " threads"
                      << std::endl;
        }

        /**
         * @brief Add a task to the executor's queue. The task will be executed by one of the worker threads.
         * @retval true if executor is running, otherwise return false.
         */
        bool post(const Task &task)
        {
            if (!running_)
            {
                return false;
            }

            tasks_.push(task);

            return true;
        }

        /**
         * Stop the executor and wait for all worker threads to finish.
         */
        void stop()
        {
            if (!running_.exchange(false))
            {
                return; // đã stop rồi
            }

            // Wake tất cả worker đang block tại waitAndPop()
            tasks_.shutdown();

            for (auto &worker : workers_)
            {
                if (worker.joinable())
                {
                    worker.join();
                }
            }

            workers_.clear();

            std::cout << "[Executor] Stopped" << std::endl;
        }

        bool isRunning() const
        {
            return running_;
        }

        ~Executor()
        {
            stop();
        }

    private:
        Executor() = default;

        /**
         * Worker thread loop.
         * Each worker thread continuously waits for tasks and executes them.
         */
        void workerLoop(size_t /*id*/)
        {
            while (running_)
            {
                auto maybeTask = tasks_.waitAndPop();

                if (!maybeTask.has_value())
                {
                    // Queue đã shutdown → thoát
                    break;
                }

                (*maybeTask)();
            }
        }

    private:
        ThreadSafeQueue<Task> tasks_;
        std::vector<std::thread> workers_;
        std::atomic<bool> running_{false};
    };

}
