//
// Created by Gianni on 4/01/2025.
//

#include "vulkan_shader.hpp"

std::vector<char> readShaderFileGLSL(const std::string& filename)
{
    std::ifstream shaderFile(filename, std::ios::ate);

    check(shaderFile.is_open(), std::format("Failed to open {}", filename).c_str());

    std::vector<char> shaderSrc(shaderFile.tellg());
    shaderFile.seekg(0);
    shaderFile.read(shaderSrc.data(), shaderSrc.size());

    return shaderSrc;
}

std::vector<uint32_t> readShaderFileSPV(const std::string& filename)
{
    std::ifstream shaderFile(filename, std::ios::binary | std::ios::ate);

    check(shaderFile.is_open(), std::format("Failed to open {}", filename).c_str());

    size_t bufferSize = shaderFile.tellg();
    std::vector<uint32_t> shaderSrc(bufferSize / 4);
    shaderFile.seekg(0);
    shaderFile.read(reinterpret_cast<char*>(shaderSrc.data()), bufferSize);

    return shaderSrc;
}

VkShaderModule createShaderModule(VkDevice device, const std::string& filename)
{
    std::vector<uint32_t> shaderSrc = readShaderFileSPV(filename);

    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo shaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderSrc.size(),
        .pCode = shaderSrc.data()
    };

    VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    vulkanCheck(result, std::format("Failed to create shader module {}", filename).c_str());

    return shaderModule;
}

std::vector<uint32_t> compileShader(const std::string& filename, shaderc_shader_kind shaderKind)
{
    // todo: check if this expects a null terminated string
    std::vector<char> shaderSrc = readShaderFileGLSL(filename);

    shaderc::Compiler compiler;
    shaderc::CompileOptions compileOptions;

    compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
    compileOptions.SetGenerateDebugInfo();

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shaderSrc.data(),
                                                                     shaderSrc.size(),
                                                                     shaderKind,
                                                                     filename.c_str(),
                                                                     compileOptions);

    check(result.GetCompilationStatus() == shaderc_compilation_status_success, result.GetErrorMessage().c_str());

    return {result.begin(), result.end()};
}

VkShaderModule compileAndCreateShaderModule(VkDevice device, const std::string& filename, shaderc_shader_kind shaderKind)
{
    std::vector<uint32_t> shaderSrc = compileShader(filename, shaderKind);

    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo shaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderSrc.size(),
        .pCode = shaderSrc.data()
    };

    VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    vulkanCheck(result, std::format("Failed to create shader module {}", filename).c_str());

    return shaderModule;
}
