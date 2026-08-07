#pragma once

#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace mini_ros
{
    /**
     * @brief using thread and await mechanism to implement a timer
     */
    class Timer
    {
    public:
        Timer(std::chrono::milliseconds interval, std::function<void()> callback)
            : interval_(interval),
              callback_(std::move(callback)),
              running_(true)
        {
            thread_ = std::thread([this]()
                                  { timerLoop(); });
        }

        ~Timer()
        {
            stop();
        }

        // Non-copyable, non-movable (owns a thread)
        Timer(const Timer &) = delete;
        Timer &operator=(const Timer &) = delete;

        /**
         * @brief stop current timer, notigy then wait for the thread to exist
         */
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

    private:
        void timerLoop()
        {
            while (true)
            {
                std::unique_lock<std::mutex> lock(mutex_);

                // wait for interval or when stop() is called
                // predicate: Has the timer been stopped?
                bool timedOut = !cv_.wait_for(lock,
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
                    // Unlock before calling callback
                    lock.unlock();
                    callback_();
                }
            }
        }

    private:
        std::chrono::milliseconds interval_;
        std::atomic<bool> running_;
        std::thread thread_;
        std::mutex mutex_;
        std::condition_variable cv_;

        // The function executed after each interval
        std::function<void()> callback_;
    };

}
