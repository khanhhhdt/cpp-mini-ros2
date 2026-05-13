#pragma once

#include <thread>
#include <chrono>
#include <functional>
#include <atomic>

namespace mini_ros
{

    class Timer
    {
    public:
        Timer(
            std::chrono::milliseconds interval,
            std::function<void()> callback)
        {
            thread_ =
                std::thread(
                    [=]()
                    {
                        while (running_)
                        {
                            std::this_thread::sleep_for(
                                interval);

                            callback();
                        }
                    });
        }

        ~Timer()
        {
            running_ = false;

            if (thread_.joinable())
            {
                thread_.join();
            }
        }

    private:
        std::thread thread_;

        std::atomic<bool> running_{true};
    };

}