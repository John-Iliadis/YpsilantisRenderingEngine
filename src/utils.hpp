//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_UTILS_HPP
#define VULKANRENDERINGENGINE_UTILS_HPP

void debugLog(const std::string& logMSG);
void check(bool result, const char* msg, std::source_location location = std::source_location::current());

#endif //VULKANRENDERINGENGINE_UTILS_HPP
