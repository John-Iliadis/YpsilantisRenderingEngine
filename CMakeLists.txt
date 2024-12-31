cmake_minimum_required(VERSION 3.28)
project(VulkanRenderingEngine)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/pch.cpp
        src/pch.hpp
)
