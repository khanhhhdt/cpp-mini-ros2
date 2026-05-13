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
        Service(const std::string &name)
            : name_(name)
        {
        }

    private:
        std::string name_;
    };

}