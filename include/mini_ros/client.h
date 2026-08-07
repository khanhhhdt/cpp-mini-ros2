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

        /**
         * @brief Synchronous execute the request.
         * @return shared pointer to the response. nullptr if service if service not yet registered.
         */
        std::shared_ptr<Res> call(std::shared_ptr<const Req> request)
        {
            return ServiceManager::instance().call<Req, Res>(name_, request);
        }

        /**
         * @brief Blocking wait for a service to register.
         * @return true if service has been registered. false if service has not been registered.
         */
        bool waitForService(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000),
                            std::chrono::milliseconds pollInterval = std::chrono::milliseconds(50))
        {
            auto deadline = std::chrono::steady_clock::now() + timeout;

            while (std::chrono::steady_clock::now() < deadline)
            {
                if (ServiceManager::instance().hasService(name_))
                {
                    return true;
                }

                std::this_thread::sleep_for(pollInterval);
            }

            return false;
        }

        /**
         * @brief Check any service that available for this client
         */
        bool serviceAvailable() const
        {
            return ServiceManager::instance().hasService(name_);
        }

        /**
         * @brief get the client name
         */
        const std::string &name() const
        {
            return name_;
        }

    private:
        std::string name_;
    };

}
