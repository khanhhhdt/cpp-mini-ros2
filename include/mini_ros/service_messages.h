#pragma once

namespace mini_ros
{

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