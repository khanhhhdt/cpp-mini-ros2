#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <thread>

#include "service_manager.h"

namespace mini_ros
{

    template <typename Req, typename Res>
    class Client
    {
    public:
        explicit Client(const std::string &name)
            : name_(name)
        {
        }

        // ==================================================
        // SYNCHRONOUS CALL
        // Trả về nullptr nếu service chưa được đăng ký.
        // ==================================================

        std::shared_ptr<Res> call(
            std::shared_ptr<const Req> request)
        {
            return ServiceManager::instance()
                .call<Req, Res>(
                    name_,
                    request);
        }

        // ==================================================
        // WAIT FOR SERVICE
        // Block cho đến khi service sẵn sàng hoặc timeout.
        // Trả về true nếu service available.
        // ==================================================

        bool waitForService(
            std::chrono::milliseconds timeout =
                std::chrono::milliseconds(5000),
            std::chrono::milliseconds pollInterval =
                std::chrono::milliseconds(50))
        {
            auto deadline =
                std::chrono::steady_clock::now() + timeout;

            while (std::chrono::steady_clock::now() < deadline)
            {
                if (ServiceManager::instance()
                        .hasService(name_))
                {
                    return true;
                }

                std::this_thread::sleep_for(pollInterval);
            }

            return false;
        }

        bool serviceAvailable() const
        {
            return ServiceManager::instance()
                .hasService(name_);
        }

        const std::string &name() const
        {
            return name_;
        }

    private:
        std::string name_;
    };

}
