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
        // Trả về false nếu executor đã stopped.
        // =============================================

        bool post(const Task &task)
        {
            if (!running_)
            {
                return false;
            }

            tasks_.push(task);

            return true;
        }

        // =============================================
        // STOP
        // FIX: Dùng tasks_.shutdown() thay vì push
        // N empty sentinel tasks. Cách cũ có race khi
        // số sentinel ít hơn số worker đang chờ.
        // =============================================

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

            std::cout
                << "[Executor] Stopped"
                << std::endl;
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

        // =============================================
        // WORKER LOOP
        // FIX: waitAndPop() trả về optional, thoát
        // khi nhận nullopt (tức là shutdown).
        // =============================================

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
        ThreadSafeQueue<Task>   tasks_;

        std::vector<std::thread> workers_;

        std::atomic<bool>       running_{ false };
    };

}
