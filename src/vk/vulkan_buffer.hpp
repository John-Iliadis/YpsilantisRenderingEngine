//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_BUFFER_HPP
#define VULKANRENDERINGENGINE_VULKAN_BUFFER_HPP

#include "vulkan_render_device.hpp"

enum class BufferType
{
    Vertex,
    Index,
    Uniform,
    Storage,
    Staging
};

enum class MemoryType
{
    Host,
    HostCoherent,
    Device
};

VkBufferUsageFlags toVkFlags(BufferType bufferType);
VkMemoryPropertyFlags toVkFlags(MemoryType memoryType);

class VulkanBuffer
{
public:
    VulkanBuffer();
    VulkanBuffer(const VulkanRenderDevice& renderDevice,
                 VkDeviceSize size,
                 BufferType bufferType,
                 MemoryType memoryType);
    VulkanBuffer(const VulkanRenderDevice& renderDevice,
                 VkDeviceSize size,
                 BufferType bufferType,
                 MemoryType memoryType,
                 const void* bufferData);
    ~VulkanBuffer();

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    void mapBufferMemory(VkDeviceSize offset, VkDeviceSize size, const void* data);
    void update(VkDeviceSize offset, VkDeviceSize size, const void* data);
    void copyBuffer(VkCommandBuffer commandBuffer, const VulkanBuffer& other, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);
    void copyBuffer(const VulkanBuffer& other, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);

    void swap(VulkanBuffer& other) noexcept;
    void setDebugName(const std::string& debugName);

    VkBuffer getBuffer() const;
    VkDeviceMemory getMemory() const;
    VkDeviceSize getSize() const;
    BufferType getBufferType() const;
    MemoryType getMemoryType() const;

private:
    VkBuffer createBuffer(BufferType type);
    VkDeviceMemory allocateBufferMemory(MemoryType type, VkBuffer buffer);

private:
    const VulkanRenderDevice* mRenderDevice;

    VkDeviceSize mSize;
    VkBuffer mBuffer;
    VkDeviceMemory mMemory;

    BufferType mType;
    MemoryType mMemoryType;
};

uint32_t getIndexCount(const VulkanBuffer& buffer);

#endif //VULKANRENDERINGENGINE_VULKAN_BUFFER_HPP
