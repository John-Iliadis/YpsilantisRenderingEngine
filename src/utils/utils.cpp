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

std::filesystem::path fileDialog(const std::string& filter)
{
    char filename[260] {};
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lpstrFile = filename;
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = filter.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileName(&ofn) == TRUE)
        return filename;
    return {};
}
