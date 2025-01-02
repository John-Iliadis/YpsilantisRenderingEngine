//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_UTILS_HPP
#define VULKANRENDERINGENGINE_UTILS_HPP

#include <vulkan/vulkan.h>
#include "../pch.hpp"

void check(bool result, const char* msg, std::source_location location = std::source_location::current());
void vulkanCheck(VkResult result, const char* msg, std::source_location location = std::source_location::current())

#endif //VULKANRENDERINGENGINE_UTILS_HPP
