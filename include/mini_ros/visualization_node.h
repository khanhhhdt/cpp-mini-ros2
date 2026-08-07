#pragma once

#include <iostream>
#include <cmath>
#include <mutex>

#include "mini_ros/node.h"
#include "mini_ros/messages.h"
#include "mini_ros/callback_group.h"

namespace mini_ros
{
    class VisualizationNode
    {
    public:
        explicit VisualizationNode(Node &node)
        {
            group_ =
                node.createCallbackGroup(
                    CallbackGroupType::Reentrant);

            // ==========================================
            // POSE SUB
            // ==========================================

            poseSub_ =
                node.createSubscriber<Pose2D>(
                    "/pose",
                    group_,
                    [this](std::shared_ptr<
                           const Pose2D>
                               pose)
                    {
                        onPose(pose);
                    });

            // ==========================================
            // MAP SUB
            // ==========================================

            mapSub_ =
                node.createSubscriber<
                    OccupancyGrid>(
                    "/map",
                    group_,
                    [this](std::shared_ptr<
                           const OccupancyGrid>
                               map)
                    {
                        onMap(map);
                    });
        }

    private:
        void onPose(
            std::shared_ptr<
                const Pose2D>
                pose)
        {
            std::cout
                << "[VIS] pose = "
                << pose->x
                << ", "
                << pose->y
                << ", "
                << pose->theta
                << std::endl;
        }

        void onMap(
            std::shared_ptr<
                const OccupancyGrid>
                map)
        {
            std::cout
                << "[VIS] map updated : "
                << map->width
                << " x "
                << map->height
                << std::endl;
        }

    private:
        CallbackGroupPtr group_;

        SubscriptionPtr poseSub_;

        SubscriptionPtr mapSub_;
    };
}