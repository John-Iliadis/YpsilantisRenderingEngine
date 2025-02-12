//
// Created by Gianni on 13/02/2025.
//

#ifndef VULKANRENDERINGENGINE_SAVE_DATA_HPP
#define VULKANRENDERINGENGINE_SAVE_DATA_HPP

#include <tiny_gltf/json.hpp>

class SaveData : public nlohmann::json
{
public:
    SaveData();
    ~SaveData();
};

#endif //VULKANRENDERINGENGINE_SAVE_DATA_HPP
