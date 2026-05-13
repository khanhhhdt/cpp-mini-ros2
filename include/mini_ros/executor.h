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

        // =============================================
        // START THREAD POOL
        // =============================================

        void start(size_t threadCount = std::thread::hardware_concurrency())
        {
            if (threadCount == 0)
            {
                threadCount = 4;
            }

            running_ = true;

            for (size_t i = 0; i < threadCount; ++i)
            {
                workers_.emplace_back(
                    [this, i]()
                    {
                        workerLoop(i);
                    });
            }

            std::cout
                << "[Executor] Started with "
                << threadCount
                << " threads"
                << std::endl;
        }

        // =============================================
        // POST TASK
        // =============================================

        void post(const Task &task)
        {
            tasks_.push(task);
        }

        // =============================================
        // STOP
        // =============================================

        void stop()
        {
            running_ = false;

            // wake all workers
            for (size_t i = 0; i < workers_.size(); ++i)
            {
                tasks_.push([]() {});
            }

            for (auto &worker : workers_)
            {
                if (worker.joinable())
                {
                    worker.join();
                }
            }

            workers_.clear();
        }

    private:
        Executor() = default;

        void workerLoop(size_t id)
        {
            while (running_)
            {
                auto task = tasks_.waitAndPop();

                task();
            }
        }

    private:
        ThreadSafeQueue<Task> tasks_;

        std::vector<std::thread> workers_;

        std::atomic<bool> running_{false};
    };

}