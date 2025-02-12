//
// Created by Gianni on 13/02/2025.
//

#include "save_data.hpp"

static const std::string sSaveDataPath = "../data/save_data.json";

SaveData::SaveData()
{
    std::ifstream file(sSaveDataPath);

    if (file.is_open())
        file >> *this;
}

SaveData::~SaveData()
{
    std::ofstream file(sSaveDataPath);
    file << dump(4);
}
