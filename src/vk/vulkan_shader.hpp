//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_SHADER_HPP
#define VULKANRENDERINGENGINE_VULKAN_SHADER_HPP

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include "vulkan_utils.hpp"
#include "../utils.hpp"

std::vector<char> readShaderFileGLSL(const std::string& filename);
std::vector<uint32_t> readShaderFileSPV(const std::string& filename);
std::vector<uint32_t> compileShader(const std::string& filename, shaderc_shader_kind shaderKind);

VkShaderModule createShaderModule(VkDevice device, const std::string& filename);
VkShaderModule compileAndCreateShaderModule(VkDevice device, const std::string& filename, shaderc_shader_kind shaderKind);

#endif //VULKANRENDERINGENGINE_VULKAN_SHADER_HPP
