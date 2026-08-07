#pragma once

#include <iostream>
#include <cmath>
#include <mutex>

#include "mini_ros/node.h"
#include "mini_ros/messages.h"
#include "mini_ros/callback_group.h"

namespace mini_ros
{

    // Example SLAM node that subscribes to LaserScan messages,
    // updates a simple occupancy grid map, and publishes the robot's pose and the map.
    class SlamNode
    {
    public:
        explicit SlamNode(Node &node)
        {
            // ==========================================
            // FIX: Dùng MutuallyExclusive thay vì Reentrant.
            // onScan() ghi vào currentPose_ và map_ —
            // không an toàn khi chạy song song nhiều callback.
            // ==========================================
            group_ = node.createCallbackGroup(CallbackGroupType::MutuallyExclusive);

            // ==========================================
            // PUBLISHERS
            // ==========================================

            posePublisher_ = std::make_shared<Publisher<Pose2D>>(node.createPublisher<Pose2D>("/pose"));

            mapPublisher_ = std::make_shared<Publisher<OccupancyGrid>>(node.createPublisher<OccupancyGrid>("/map"));

            // ==========================================
            // SUBSCRIBER
            // ==========================================

            scanSubscription_ = node.createSubscriber<LaserScan>("/scan",
                                                                 group_,
                                                                 [this](std::shared_ptr<const LaserScan> scan)
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
            map_ = std::make_shared<OccupancyGrid>();
            map_->width = 100;
            map_->height = 100;
            map_->resolution = 0.05f;
            map_->data.resize(map_->width * map_->height, -1);
        }

        // ==============================================
        // PROCESS SCAN
        // Chạy dưới MutuallyExclusive group nên
        // currentPose_ và map_ được bảo vệ.
        // ==============================================

        void onScan(std::shared_ptr<const LaserScan> scan)
        {
            std::cout << "[SLAM] processing scan..."
                      << std::endl;

            // Fake motion update
            currentPose_.x += 0.05f;
            currentPose_.theta += 0.01f;
            currentPose_.header.timestamp = nowNs();

            updateMap(scan);

            // ==========================================
            // PUBLISH POSE
            // ==========================================

            auto poseMsg = std::make_shared<Pose2D>(currentPose_);

            posePublisher_->publish(poseMsg);

            // ==========================================
            // PUBLISH MAP
            // ==========================================

            mapPublisher_->publish(map_);
        }

        // ==============================================
        // SIMPLE OCCUPANCY UPDATE
        // FIX: Thay magic number 10.0f bằng 1.0f/resolution
        // để đúng với map resolution = 0.05m/cell
        // (1 meter = 20 cells khi resolution=0.05)
        // ==============================================

        void updateMap(std::shared_ptr<const LaserScan> scan)
        {
            const int centerX = map_->width / 2;

            const int centerY = map_->height / 2;

            // FIX: Dùng resolution thay vì hardcode
            const float metersToCell = 1.0f / map_->resolution;

            float angle = scan->angleMin;

            for (const auto &range : scan->ranges)
            {
                const int hitX = centerX + static_cast<int>(std::cos(angle) * range * metersToCell);
                const int hitY = centerY + static_cast<int>(std::sin(angle) * range * metersToCell);

                if (hitX >= 0 &&
                    hitX < map_->width &&
                    hitY >= 0 &&
                    hitY < map_->height)
                {
                    map_->data[hitY * map_->width + hitX] = 100;
                }

                angle += scan->angleIncrement;
            }
        }

    private:
        CallbackGroupPtr group_;

        std::shared_ptr<Publisher<Pose2D>> posePublisher_;

        std::shared_ptr<Publisher<OccupancyGrid>> mapPublisher_;

        SubscriptionPtr scanSubscription_;

        Pose2D currentPose_;

        std::shared_ptr<OccupancyGrid> map_;
    };

}
