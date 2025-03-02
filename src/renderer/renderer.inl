template <typename T>
void addLight(std::unordered_map<uuid32_t, index_t>& idToIndexMap,
              std::vector<T>& lightVec,
              VulkanBuffer& ssbo,
              uuid32_t id,
              const T& light)
{
    idToIndexMap.emplace(id, lightVec.size());
    lightVec.push_back(light);

    uint32_t lastIndex = lightVec.size() - 1;
    ssbo.update(lastIndex * sizeof(T), sizeof(T), &lightVec.at(lastIndex));
}

template <typename T>
T& getLight(std::unordered_map<uuid32_t, index_t>& idToIndexMap,
                   std::vector<T>& lightVec,
                   uuid32_t id)
{
    return lightVec.at(idToIndexMap.at(id));
}

template <typename T>
void updateLight(std::unordered_map<uuid32_t, index_t>& idToIndexMap,
                 std::vector<T>& lightVec,
                 VulkanBuffer& ssbo,
                 uuid32_t id)
{
    index_t index = idToIndexMap.at(id);
    T& light = getLight(idToIndexMap, lightVec, id);
    ssbo.update(index * sizeof(T), sizeof(T), &light);
}

template <typename T>
void deleteLight(std::unordered_map<uuid32_t, index_t>& idToIndexMap,
                 std::vector<T>& lightVec,
                 VulkanBuffer& ssbo,
                 uuid32_t id)
{
    index_t removeIndex = idToIndexMap.at(id);
    index_t lastIndex = lightVec.size() - 1;

    idToIndexMap.erase(id);

    if (removeIndex != lastIndex)
    {
        std::swap(lightVec.at(removeIndex), lightVec.at(lastIndex));

        for (auto& pair : idToIndexMap)
        {
            if (pair.second == lastIndex)
            {
                pair.second = removeIndex;
                break;
            }
        }

        ssbo.copyBuffer(ssbo,
                        lastIndex * sizeof(T),
                        removeIndex * sizeof(T),
                        sizeof(T));
    }

    lightVec.pop_back();
}
