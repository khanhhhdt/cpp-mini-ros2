#pragma once

#include <memory>
#include <string>

#include "service_manager.h"

namespace mini_ros
{

    template <typename Req, typename Res>
    class Client
    {
    public:
        Client(const std::string &name)
            : name_(name)
        {
        }

        std::shared_ptr<Res> call(
            std::shared_ptr<const Req> request)
        {
            return ServiceManager::instance()
                .call<Req, Res>(
                    name_,
                    request);
        }

    private:
        std::string name_;
    };

}