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

    return std::string_view(str.end() - end.length(), str.end()) == end;
}

void removeEndNullChars(std::vector<char>& vec)
{
    for (size_t i = vec.size() - 1; i > 0; --i)
    {
        if (vec.at(i) != '\0')
            break;
        vec.pop_back();
    }
}
