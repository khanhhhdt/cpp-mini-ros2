#pragma once

namespace mini_ros
{
    /**
     * Example of service
     */
    struct AddTwoIntsRequest
    {
        int a;
        int b;
    };

    struct AddTwoIntsResponse
    {
        int sum;
    };

}
