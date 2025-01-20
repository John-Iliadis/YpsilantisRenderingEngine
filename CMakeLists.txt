cmake_minimum_required(VERSION 3.27.8)
project(VulkanRenderingEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(DEPENDENCY_DIR ${CMAKE_SOURCE_DIR}/dependencies)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "MinGW compiler required")
endif()

file(GLOB INCLUDE_DIRS ${DEPENDENCY_DIR}/*/include)
file(GLOB LIBS ${DEPENDENCY_DIR}/*/lib/*.a)
file(GLOB IMGUI_SRC ${DEPENDENCY_DIR}/imgui/include/imgui/*.cpp)

add_executable(${PROJECT_NAME}
        ${IMGUI_SRC}
        src/main.cpp
        src/pch.cpp
        src/application.cpp
        src/application.hpp
        src/utils.cpp
        src/utils.hpp
        src/window/window.cpp
        src/window/window.hpp
        src/window/Event.hpp
        src/window/input.cpp
        src/window/input.hpp
        src/vk/vulkan_instance.cpp
        src/vk/vulkan_instance.hpp
        src/vk/include.hpp
        src/vk/vulkan_render_device.cpp
        src/vk/vulkan_render_device.hpp
        src/vk/vulkan_swapchain.cpp
        src/vk/vulkan_swapchain.hpp
        src/vk/vulkan_image.cpp
        src/vk/vulkan_image.hpp
        src/vk/vulkan_buffer.cpp
        src/vk/vulkan_buffer.hpp
        src/vk/vulkan_utils.cpp
        src/vk/vulkan_utils.hpp
        src/vk/vulkan_texture.cpp
        src/vk/vulkan_texture.hpp
        src/renderer/camera.cpp
        src/renderer/camera.hpp
        src/renderer/renderer.cpp
        src/renderer/renderer.hpp
        src/vk/vulkan_shader.cpp
        src/vk/vulkan_shader.hpp
        src/vk/vulkan_descriptor.cpp
        src/vk/vulkan_descriptor.hpp
        src/vk/vulkan_pipeline.cpp
        src/vk/vulkan_pipeline.hpp
        src/vk/vulkan_function_pointers.hpp
        src/vk/vulkan_imgui.cpp
        src/vk/vulkan_imgui.hpp
        src/renderer/scene.hpp
        src/renderer/scene.cpp
        src/renderer/instancedMesh.hpp
        src/renderer/instancedMesh.cpp
        src/renderer/scene_node.cpp
        src/renderer/scene_node.hpp
        src/renderer/model.hpp
        src/renderer/material.hpp
        src/renderer/model.cpp
        src/renderer/importer/loaded_resource.hpp
        src/renderer/importer/model_importer.cpp
        src/renderer/importer/model_importer.hpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX -d)

find_package(Vulkan REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan
        $ENV{VULKAN_SDK}/Lib/shaderc_shared.lib
        ${LIBS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEBUG_MODE>
        $<$<CONFIG:Debug>:SHADER_PATH=\"${CMAKE_SOURCE_DIR}/shaders/\">
        $<$<CONFIG:Release>:SHADER_PATH=\"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/\">
        GLFW_EXPOSE_NATIVE_WIN32
        GLFW_INCLUDE_VULKAN
        STB_IMAGE_IMPLEMENTATION
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
        IMGUI_DEFINE_MATH_OPERATORS
        MAX_FRAMES_IN_FLIGHT=2
)

target_link_options(${PROJECT_NAME} PRIVATE -static)
