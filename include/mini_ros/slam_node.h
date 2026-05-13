#pragma once

#include <iostream>
#include <cmath>

#include "mini_ros/node.h"
#include "mini_ros/messages.h"
#include "mini_ros/callback_group.h"

namespace mini_ros
{

    class SlamNode
    {
    public:
        explicit SlamNode(Node &node)
        {
            // ==========================================
            // CALLBACK GROUP
            // ==========================================

            group_ =
                node.createCallbackGroup(
                    CallbackGroupType::Reentrant);

            // ==========================================
            // PUBLISHERS
            // ==========================================

            posePublisher_ =
                std::make_shared<
                    Publisher<Pose2D>>(
                    node.createPublisher<Pose2D>(
                        "/pose"));

            mapPublisher_ =
                std::make_shared<
                    Publisher<OccupancyGrid>>(
                    node.createPublisher<
                        OccupancyGrid>(
                        "/map"));

            // ==========================================
            // SUBSCRIBER
            // ==========================================

            scanSubscription_ =
                node.createSubscriber<LaserScan>(
                    "/scan",
                    group_,
                    [this](std::shared_ptr<
                           const LaserScan>
                               scan)
                    {
                        onScan(scan);
                    });

            createMap();
        }

    private:
        // ==============================================
        // CREATE MAP
        // ==============================================

        void createMap()
        {
            map_ =
                std::make_shared<
                    OccupancyGrid>();

            map_->width = 100;

            map_->height = 100;

            map_->resolution = 0.05f;

            map_->data.resize(
                map_->width *
                    map_->height,
                -1);
        }

        // ==============================================
        // PROCESS SCAN
        // ==============================================

        void onScan(
            std::shared_ptr<
                const LaserScan>
                scan)
        {
            std::cout
                << "[SLAM] processing scan..."
                << std::endl;

            // fake motion

            currentPose_.x += 0.05f;

            currentPose_.theta += 0.01f;

            updateMap(scan);

            // ==========================================
            // PUBLISH POSE
            // ==========================================

            auto poseMsg =
                std::make_shared<Pose2D>();

            *poseMsg =
                currentPose_;

            posePublisher_->publish(
                poseMsg);

            // ==========================================
            // PUBLISH MAP
            // ==========================================

            mapPublisher_->publish(
                map_);
        }

        // ==============================================
        // SIMPLE OCCUPANCY UPDATE
        // ==============================================

        void updateMap(
            std::shared_ptr<
                const LaserScan>
                scan)
        {
            int centerX =
                map_->width / 2;

            int centerY =
                map_->height / 2;

            float angle =
                scan->angleMin;

            for (const auto &range :
                 scan->ranges)
            {
                int hitX =
                    centerX +
                    static_cast<int>(
                        std::cos(angle) *
                        range * 10.0f);

                int hitY =
                    centerY +
                    static_cast<int>(
                        std::sin(angle) *
                        range * 10.0f);

                if (hitX >= 0 &&
                    hitX < map_->width &&
                    hitY >= 0 &&
                    hitY < map_->height)
                {
                    int index =
                        hitY *
                            map_->width +
                        hitX;

                    map_->data[index] =
                        100;
                }

                angle +=
                    scan->angleIncrement;
            }
        }

    private:
        CallbackGroupPtr group_;

        std::shared_ptr<
            Publisher<Pose2D>>
            posePublisher_;

        std::shared_ptr<
            Publisher<OccupancyGrid>>
            mapPublisher_;

        SubscriptionPtr
            scanSubscription_;

        Pose2D currentPose_;

        std::shared_ptr<
            OccupancyGrid>
            map_;
    };

}