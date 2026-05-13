#pragma once

#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace mini_ros
{

    // ===========================================================
    // FIX: Timer cũ dùng sleep_for() nên destructor bị block
    // cho đến hết interval. Version mới dùng condition_variable
    // với wait_for() → stop() wake ngay lập tức.
    // ===========================================================

    class Timer
    {
    public:
        Timer(
            std::chrono::milliseconds interval,
            std::function<void()> callback)
            : interval_(interval),
              callback_(std::move(callback)),
              running_(true)
        {
            thread_ =
                std::thread(
                    [this]()
                    {
                        timerLoop();
                    });
        }

        // -----------------------------------------------------------
        // Stop timer và join ngay, không chờ hết interval.
        // -----------------------------------------------------------

        void stop()
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);

                running_ = false;
            }

            cv_.notify_one();

            if (thread_.joinable())
            {
                thread_.join();
            }
        }

        ~Timer()
        {
            stop();
        }

        // Non-copyable, non-movable (owns a thread)
        Timer(const Timer &)            = delete;
        Timer &operator=(const Timer &) = delete;

    private:
        void timerLoop()
        {
            while (true)
            {
                std::unique_lock<std::mutex> lock(mutex_);

                // Chờ đúng interval hoặc đến khi stop() được gọi
                bool timedOut =
                    !cv_.wait_for(
                        lock,
                        interval_,
                        [this]()
                        {
                            return !running_;
                        });

                if (!running_)
                {
                    break;
                }

                if (timedOut)
                {
                    // Unlock trước khi gọi callback tránh hold lock
                    lock.unlock();

                    callback_();
                }
            }
        }

    private:
        std::chrono::milliseconds   interval_;

        std::function<void()>       callback_;

        std::atomic<bool>           running_;

        std::thread                 thread_;

        std::mutex                  mutex_;

        std::condition_variable     cv_;
    };

}
