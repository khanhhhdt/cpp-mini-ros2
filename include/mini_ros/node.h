#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <string>
#include <vector>

#include "topic_manager.h"
#include "publisher.h"
#include "subscription.h"
#include "callback_group.h"
#include "service.h"
#include "client.h"
#include "service_manager.h"

namespace mini_ros
{

    class Node
    {
    public:
        explicit Node(const std::string &name)
            : name_(name)
        {
        }

        const std::string &name() const
        {
            return name_;
        }

        // ==================================================
        // CREATE PUBLISHER
        // ==================================================

        template <typename T>
        Publisher<T> createPublisher(
            const std::string &topic)
        {
            return Publisher<T>(topic);
        }

        // ==================================================
        // CREATE SUBSCRIBER — generic callback version
        // (lambda, functor, free function)
        // ==================================================

        template <
            typename T,
            typename Callback>
        SubscriptionPtr createSubscriber(
            const std::string &topic,
            CallbackGroupPtr group,
            Callback &&callback)
        {
            auto subscription =
                std::make_shared<Subscription>();

            auto wrapper =
                [group,
                 cb = std::forward<Callback>(callback)](std::shared_ptr<const T> msg) mutable
            {
                if (group &&
                    group->type() ==
                        CallbackGroupType::MutuallyExclusive)
                {
                    std::lock_guard<std::mutex>
                        lock(group->mutex());

                    cb(msg);
                }
                else
                {
                    cb(msg);
                }
            };

            TopicManager::instance()
                .subscribe<T>(
                    topic,
                    wrapper,
                    subscription);

            return subscription;
        }

        // ==================================================
        // CREATE SUBSCRIBER — member function safe version
        // Dùng weak_ptr để tránh dangling pointer khi object bị destroy
        // ==================================================

        template <
            typename T,
            typename Obj>
        SubscriptionPtr createSubscriber(
            const std::string &topic,
            void (Obj::*method)(
                std::shared_ptr<const T>),
            std::shared_ptr<Obj> object)
        {
            auto subscription =
                std::make_shared<Subscription>();

            std::weak_ptr<Obj> weakObject =
                object;

            auto wrapper =
                [weakObject, method](std::shared_ptr<const T> msg)
            {
                auto shared =
                    weakObject.lock();

                if (!shared)
                {
                    return;
                }

                ((*shared).*method)(msg);
            };

            TopicManager::instance()
                .subscribe<T>(
                    topic,
                    wrapper,
                    subscription);

            return subscription;
        }

        // ==================================================
        // CREATE CALLBACK GROUP
        // ==================================================

        CallbackGroupPtr createCallbackGroup(
            CallbackGroupType type)
        {
            return std::make_shared<CallbackGroup>(type);
        }

        // ==================================================
        // CREATE SERVICE
        // FIX: Trả về ServicePtr thay vì void để caller có
        // thể quản lý lifetime. Service tự unregister khi bị destroy.
        // ==================================================

        template <
            typename Req,
            typename Res,
            typename Callback>
        std::shared_ptr<Service<Req, Res>> createService(
            const std::string &name,
            Callback &&callback)
        {
            ServiceManager::instance()
                .registerService<Req, Res>(
                    name,
                    std::forward<Callback>(callback));

            return std::make_shared<Service<Req, Res>>(name);
        }

        // ==================================================
        // CREATE CLIENT
        // ==================================================

        template <typename Req, typename Res>
        Client<Req, Res> createClient(
            const std::string &name)
        {
            return Client<Req, Res>(name);
        }

    private:
        std::string name_;
    };

}
