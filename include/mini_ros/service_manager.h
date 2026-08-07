#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace mini_ros
{
    /**
     * @brief Manage and execute the service callback
     */
    class ServiceManager
    {
    public:
        static ServiceManager &instance()
        {
            static ServiceManager manager;

            return manager;
        }

        template <typename Req, typename Res>
        void registerService(const std::string &name,
                             std::function<void(std::shared_ptr<const Req>, std::shared_ptr<Res>)> callback)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto wrapper = [callback](std::shared_ptr<const void> req,
                                      std::shared_ptr<void> res)
            {
                callback(std::static_pointer_cast<const Req>(req),
                         std::static_pointer_cast<Res>(res));
            };

            services_[name] = wrapper;
        }

        /**
         * @brief Unregister the service when the service is destryed
         * @param name Service's name
         * @retval none
         */
        void unregisterService(const std::string &name)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            services_.erase(name);
        }

        /**
         * @brief Execute a specific service by it name
         * @param name service name
         * @param request supply the request parameter
         * @retval Res pointer that execute in the callback
         */
        template <typename Req, typename Res>
        std::shared_ptr<Res> call(const std::string &name, std::shared_ptr<const Req> request)
        {
            ServiceCallback callback;

            {
                std::lock_guard<std::mutex> lock(mutex_);

                auto it = services_.find(name);

                if (it == services_.end())
                {
                    return nullptr;
                }

                callback = it->second;
            }

            auto response = std::make_shared<Res>();

            callback(request, response);

            return response;
        }

        /**
         * @brief Check a service exist in the service manager
         * @param name name of the service
         */
        bool hasService(const std::string &name) const
        {
            std::lock_guard<std::mutex> lock(mutex_);

            return services_.count(name) > 0;
        }

    private:
        using ServiceCallback = std::function<void(std::shared_ptr<const void>,
                                                   std::shared_ptr<void>)>;

        std::unordered_map<std::string, ServiceCallback> services_;

        mutable std::mutex mutex_;
    };

}
