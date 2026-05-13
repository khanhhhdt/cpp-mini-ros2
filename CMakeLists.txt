cmake_minimum_required(VERSION 3.10)

project(mini_ros)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(
    mini_ros

    src/main.cpp 
)