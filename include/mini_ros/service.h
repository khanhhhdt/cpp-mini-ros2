#pragma once

#include <memory>
#include <string>

#include "service_manager.h"

namespace mini_ros
{
    template <typename Req, typename Res>
    class Service
    {
    public:
        explicit Service(const std::string &name)
            : name_(name)
        {
        }

        ~Service()
        {
            // auto unregister when service destry
            ServiceManager::instance().unregisterService(name_);
        }

        const std::string &name() const
        {
            return name_;
        }

        // Non-copyable (ownership)
        Service(const Service &) = delete;
        Service &operator=(const Service &) = delete;

        // Movable
        Service(Service &&) = default;
        Service &operator=(Service &&) = default;

    private:
        std::string name_;
    };

    template <typename Req, typename Res>
    using ServicePtr = std::shared_ptr<Service<Req, Res>>;

}
