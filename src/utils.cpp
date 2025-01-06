//
// Created by Gianni on 2/01/2025.
//

#include "utils.hpp"

void debugLog(const std::string& logMSG)
{
    std::cout << "[Debug Log] " << logMSG << std::endl;
}

void check(bool result, const char* msg, std::source_location location)
{
    if (!result)
    {
        std::stringstream errorMSG;
        errorMSG << '`' << location.function_name() << "`: " << msg << '\n';

        throw std::runtime_error(errorMSG.str());
    }
}

size_t stringLength(const char* str)
{
    size_t length = 0;

    while (str[length])
        ++length;

    return length;
}

bool endsWidth(const std::string& str, const std::string& end)
{
    if (str.length() < end.length())
        return false;

    return std::string_view(str.begin() + end.length() - 1, str.end()) == end;
}
