//
// Created by Gianni on 4/01/2025.
//

#include "vulkan_shader.hpp"

static std::string sGetShaderPath(const std::string& shaderName)
{
#ifdef DEBUG_MODE
    return SHADER_PATH + shaderName;
#else
    return SHADER_PATH + shaderName + ".spv";
#endif
}

VulkanShaderModule::VulkanShaderModule(VkDevice device, const std::string &shaderName)
    : mDevice(device)
{
    std::string shaderPath = sGetShaderPath(shaderName);
    createShaderModule(getShaderSrc(shaderPath), shaderName);
}

VulkanShaderModule::~VulkanShaderModule()
{
    vkDestroyShaderModule(mDevice, mShaderModule, nullptr);
}

VulkanShaderModule::operator VkShaderModule() const
{
    return mShaderModule;
}

void VulkanShaderModule::createShaderModule(const std::vector<uint32_t> &shaderSrc,
                                            const std::string& shaderName)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderSrc.size(),
        .pCode = shaderSrc.data()
    };

    VkResult result = vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mShaderModule);
    vulkanCheck(result, "Failed to create shader module.");

    setDebugVulkanObjectName(mDevice,
                             VK_OBJECT_TYPE_SHADER_MODULE,
                             shaderName.c_str(),
                             mShaderModule);
}

std::vector<char> VulkanShaderModule::readShaderFileGLSL(const std::string &shaderPath)
{
    std::ifstream shaderFile(shaderPath, std::ios::ate);

    check(shaderFile.is_open(), std::format("Failed to open {}", shaderPath).c_str());

    std::vector<char> shaderSrc(shaderFile.tellg());
    shaderFile.seekg(0);
    shaderFile.read(shaderSrc.data(), shaderSrc.size());

    return shaderSrc;
}

std::vector<uint32_t> VulkanShaderModule::readShaderFileSPV(const std::string &shaderPath)
{
    std::ifstream shaderFile(shaderPath, std::ios::binary | std::ios::ate);

    check(shaderFile.is_open(), std::format("Failed to open {}", shaderPath).c_str());

    size_t bufferSize = shaderFile.tellg();
    std::vector<uint32_t> shaderSrc(bufferSize / 4);
    shaderFile.seekg(0);
    shaderFile.read(reinterpret_cast<char*>(shaderSrc.data()), bufferSize);

    return shaderSrc;
}

std::vector<uint32_t> VulkanShaderModule::compileShader(const std::string &shaderPath)
{
    // todo: check if this expects a null terminated string
    std::vector<char> shaderSrc = readShaderFileGLSL(shaderPath);

    shaderc::Compiler compiler;
    shaderc::CompileOptions compileOptions;

    compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
    compileOptions.SetGenerateDebugInfo();

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shaderSrc.data(),
                                                                     shaderSrc.size(),
                                                                     getShaderKind(shaderPath),
                                                                     shaderPath.c_str(),
                                                                     compileOptions);

    check(result.GetCompilationStatus() == shaderc_compilation_status_success, result.GetErrorMessage().c_str());

    return {result.begin(), result.end()};
}

std::vector<uint32_t> VulkanShaderModule::getShaderSrc(const std::string &shaderPath)
{
#ifdef DEBUG_MODE
    return compileShader(shaderPath);
#else
    return readShaderFileSPV(shaderPath);
#endif
}

shaderc_shader_kind VulkanShaderModule::getShaderKind(const std::string &shaderPath)
{
    if (endsWidth(shaderPath, "vert"))
        return shaderc_vertex_shader;
    else if (endsWidth(shaderPath, "frag"))
        return shaderc_fragment_shader;
    else if (endsWidth(shaderPath, "comp"))
        return shaderc_compute_shader;
    else if (endsWidth(shaderPath, "geom"))
        return shaderc_geometry_shader;
    else if (endsWidth(shaderPath, "tcs"))
        return shaderc_tess_control_shader;
    else if (endsWidth(shaderPath, "tes"))
        return shaderc_tess_evaluation_shader;

    check(false, "Case not implemented.");
    return shaderc_vertex_shader; // to get rid of warning
}
