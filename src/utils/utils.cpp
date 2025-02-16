//
// Created by Gianni on 2/01/2025.
//

#include "utils.hpp"
#include "../window/window.hpp"

void debugLog(const std::string& logMSG)
{
    std::cout << "[Debug Log] " << logMSG << std::endl;
}

void check(bool result, const char* msg, std::source_location location)
{
    if (!result)
    {
        std::stringstream errorMSG;
        errorMSG << '`' << location.function_name() << "`: " << msg;

        throw std::runtime_error(errorMSG.str());
    }
}

static HWND hwnd = nullptr;
void setHWND(HWND windowHandle) { hwnd = windowHandle; }
HWND getHWND() { return hwnd; }

std::filesystem::path fileDialog(const char* title, const char* filter)
{
    char filename[1024] {};

    OPENFILENAME ofn {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = hwnd,
        .hInstance = (HINSTANCE)GetModuleHandle(nullptr),
        .lpstrFilter = filter,
        .lpstrFile = filename,
        .nMaxFile = sizeof(filename),
        .lpstrTitle = title,
        .Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR
    };

    if (GetOpenFileName(&ofn) == TRUE)
        return filename;
    return {};
}

std::string fileExtension(const std::filesystem::path& path)
{
    std::string extension = path.extension().string();

    std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

    return extension;
}
