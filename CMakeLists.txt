cmake_minimum_required(VERSION 3.28)
project(VulkanRenderingEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(DEPENDENCY_DIR ${CMAKE_SOURCE_DIR}/dependencies)

file(GLOB INCLUDE_DIRS ${DEPENDENCY_DIR}/*/include)
file(GLOB LIBS ${DEPENDENCY_DIR}/*/lib/*.a)
file(GLOB IMGUI_SRC ${IMGUI_DIR}/include/imgui/*.cpp)

add_executable(${PROJECT_NAME}
        ${IMGUI_SRC}
        src/main.cpp
        src/pch.cpp
        src/pch.hpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX -d)

find_package(Vulkan REQUIRED)
message(STATUS "Vulkan include directory: ${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan libraries: ${Vulkan_LIBRARIES}")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIR}
    ${INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Vulkan_LIBRARIES}
    ${LIBS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLFW_INCLUDE_VULKAN
    STB_IMAGE_IMPLEMENTATION
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
    GLM_FORCE_LEFT_HANDED
    $<$<CONFIG:Debug>:DEBUG_MODE>
)
