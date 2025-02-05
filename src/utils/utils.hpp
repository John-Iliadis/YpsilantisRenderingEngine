//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_UTILS_HPP
#define VULKANRENDERINGENGINE_UTILS_HPP

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

void debugLog(const std::string& logMSG);

void check(bool result, const char* msg, std::source_location location = std::source_location::current());

void setHWND(HWND windowHandle);
HWND getHWND();

std::filesystem::path fileDialog(const std::string& filter);

std::string fileExtension(const std::filesystem::path& path);

struct cStrHash
{
    size_t operator()(const char* str) const
    {
        size_t result = 0;
        size_t len = strlen(str);

        for (size_t i = 0; i < len; ++i)
            result = str[i] + (result * 31);

        return result;
    }
};

struct cStrCompare
{
    bool operator()(const char* str1, const char* str2) const
    {
        size_t len1 = strlen(str1);
        size_t len2 = strlen(str2);

        if (len1 != len2)
            return false;

        for (size_t i = 0; i < len1; ++i)
            if (str1[i] != str2[i])
                return false;
        return true;
    }
};

#endif //VULKANRENDERINGENGINE_UTILS_HPP
