//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_SHADER_HPP
#define VULKANRENDERINGENGINE_VULKAN_SHADER_HPP

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include "vulkan_render_device.hpp"
#include "vulkan_utils.hpp"
#include "../utils.hpp"

class VulkanShaderModule
{
public:
    void create(const VulkanRenderDevice& renderDevice, const std::string& shaderName);
    void destroy(const VulkanRenderDevice& renderDevice);

    operator VkShaderModule() const;

private:
    void createShaderModule(const VulkanRenderDevice& renderDevice,
                            const std::vector<uint32_t>& shaderSrc,
                            const std::string& shaderName);
    std::vector<char> readShaderFileGLSL(const std::string& shaderPath);
    std::vector<uint32_t> readShaderFileSPV(const std::string& shaderPath);
    std::vector<uint32_t> compileShader(const std::string& shaderPath);
    std::vector<uint32_t> getShaderSrc(const std::string& shaderPath);
    shaderc_shader_kind getShaderKind(const std::string& shaderPath);

private:
    VkShaderModule mShaderModule;
};

#endif //VULKANRENDERINGENGINE_VULKAN_SHADER_HPP
