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
        src/app/application.cpp
        src/app/application.hpp
        src/utils/utils.cpp
        src/utils/utils.hpp
        src/window/window.cpp
        src/window/window.hpp
        src/window/Event.hpp
        src/window/input.cpp
        src/window/input.hpp
        src/vk/vulkan_instance.cpp
        src/vk/vulkan_instance.hpp
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
        src/vk/vulkan_descriptor.cpp
        src/vk/vulkan_descriptor.hpp
        src/vk/vulkan_pipeline.cpp
        src/vk/vulkan_pipeline.hpp
        src/vk/vulkan_function_pointers.hpp
        src/vk/vulkan_imgui.cpp
        src/vk/vulkan_imgui.hpp
        src/renderer/instanced_mesh.hpp
        src/renderer/instanced_mesh.cpp
        src/renderer/model.hpp
        src/renderer/model.cpp
        src/app/types.hpp
        src/app/simple_notification_service.hpp
        src/app/simple_notification_service.cpp
        src/app/uuid_registry.hpp
        src/app/uuid_registry.cpp
        src/app/editor.cpp
        src/app/editor.hpp
        src/renderer/vertex.hpp
        src/utils/loaded_image.cpp
        src/utils/loaded_image.hpp
        src/utils/main_thread_task_queue.cpp
        src/utils/main_thread_task_queue.hpp
        src/renderer/model_importer.cpp
        src/renderer/model_importer.hpp
        src/scene_graph/scene_graph.cpp
        src/scene_graph/scene_graph.hpp
        src/scene_graph/graph_node.cpp
        src/scene_graph/graph_node.hpp
        src/app/save_data.cpp
        src/app/save_data.hpp
        src/utils/timer.cpp
        src/utils/timer.hpp
        src/vk/vulkan_renderpass.cpp
        src/vk/vulkan_renderpass.hpp
        src/renderer/material.hpp
        src/renderer/material.cpp
        src/renderer/lights.hpp
)

add_subdirectory(${DEPENDENCY_DIR}/stb/)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX -d)

find_package(Vulkan REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan stb ${LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEBUG_MODE>
        GLFW_EXPOSE_NATIVE_WIN32
        GLFW_INCLUDE_VULKAN
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
        GLM_ENABLE_EXPERIMENTAL
        IMGUI_DEFINE_MATH_OPERATORS
)

target_link_options(${PROJECT_NAME} PRIVATE -static)

# compile shaders
set(SHADER_DIR ${PROJECT_SOURCE_DIR}/shaders)
set(COMPILED_SHADER_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
set(GLSLC_EXE ${PROJECT_SOURCE_DIR}/dependencies/glslc/glslc.exe)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(GEN_DEBUG_INFO_FLAG "-g")
endif()

file(GLOB GLSL_FILES
        ${SHADER_DIR}/*.vert
        ${SHADER_DIR}/*.frag
        ${SHADER_DIR}/*.geom
        ${SHADER_DIR}/*.comp
)

foreach (GLSL_FILE ${GLSL_FILES})
    get_filename_component(FILENAME ${GLSL_FILE} NAME)
    set(SPIRV_FILE ${COMPILED_SHADER_DIR}/${FILENAME}.spv)

    add_custom_command(
            OUTPUT ${SPIRV_FILE}
            COMMAND ${GLSLC_EXE} ${GEN_DEBUG_INFO_FLAG} ${GLSL_FILE} -o ${SPIRV_FILE}
            DEPENDS ${GLSL_FILE}
            COMMENT "Compiling ${GLSL_FILE} to SPIR-V..."
            VERBATIM
    )

    list(APPEND SPIRV_FILES ${SPIRV_FILE})
endforeach ()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_FILES})
