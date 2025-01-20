//
// Created by Gianni on 21/01/2025.
//

#ifndef VULKANRENDERINGENGINE_DYNAMIC_BUFFER_HPP
#define VULKANRENDERINGENGINE_DYNAMIC_BUFFER_HPP

#include <vulkan/vulkan.h>
#include "../vk/include.hpp"

// todo: remove
#include <vector>
#include <array>
#include <optional>

template<typename T>
class DynamicBuffer
{
public:
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
    std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT> buffers;
    std::vector<T> data;

public:
    DynamicBuffer();

    void create(const VulkanRenderDevice& renderDevice,
                VkDescriptorSetLayout layout,
                const std::string& descriptorDebugName = "",
                const std::string& buffersDebugName = "");
    void destroy();

    void add(const T& item);
    void add(T&& item);
    void update(uint32_t index);
    void remove(const T& item);

    void updateBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex);

private:
    std::optional<uint32_t> search(const T* item) const;

private:
    const VulkanRenderDevice* mRenderDevice;

    std::array<std::vector<T>, MAX_FRAMES_IN_FLIGHT> mAddPending;
    std::array<std::vector<uint32_t>, MAX_FRAMES_IN_FLIGHT> mUpdatePending;
    std::array<std::vector<uint32_t>, MAX_FRAMES_IN_FLIGHT> mRemovePending;

    static const uint32_t sInitialBufferSize = 32;
};

template<typename T>
DynamicBuffer<T>::DynamicBuffer()
    : descriptorSets()
    , buffers()
    , mRenderDevice()
{
}

template<typename T>
void DynamicBuffer<T>::create(const VulkanRenderDevice &renderDevice, VkDescriptorSetLayout layout,
                              const std::string &descriptorDebugName, const std::string &buffersDebugName)
{
    mRenderDevice = &renderDevice;

    DescriptorSetBuilder dsBuilder;
    dsBuilder.init(renderDevice.device, renderDevice.descriptorPool);

    // TODO: MOVE THIS INTO A FUNCTION
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        descriptorSets.at(i) = dsBuilder
            .setLayout(layout)
            .addBuffer()

    }

}

template<typename T>
void DynamicBuffer<T>::destroy()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        destroyBuffer(*mRenderDevice, buffers.at(i));
}

template<typename T>
void DynamicBuffer<T>::add(const T &item)
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        mAddPending.at(i).push_back(item);
}

template<typename T>
void DynamicBuffer<T>::add(T &&item)
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        mAddPending.at(i).push_back(std::move(item));
}

template<typename T>
void DynamicBuffer<T>::update(uint32_t index)
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        mUpdatePending.at(i).push_back(i);
}

template<typename T>
void DynamicBuffer<T>::remove(const T& item)
{
    std::optional<uint32_t> index = search(&item);

    if (index.has_value())
    {
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            mRemovePending.at(i).push_back(index.value());
    }
}

template<typename T>
void DynamicBuffer<T>::updateBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{

}

template<typename T>
std::optional<uint32_t> DynamicBuffer<T>::search(const T *item) const
{
    // check if item is part of the vector
    if (item < data.begin() || item >= data.end())
        return {};

    // binary search
    uint32_t left = 0;
    uint32_t right = data.size() - 1;

    while (right >= left)
    {
        uint32_t current = (left + right) / 2;

        const T* dataCurrent = &data.at(current);

        if (dataCurrent == item)
            return current;

        if (dataCurrent < item)
            right = current - 1;
        else
            left = current + 1;
    }

    return {};
}

#endif //VULKANRENDERINGENGINE_DYNAMIC_BUFFER_HPP
