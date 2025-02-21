//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_buffer.hpp"

static void mapBufferMemory(const VulkanRenderDevice* renderDevice,
                            VkDeviceMemory memory,
                            VkDeviceSize offset,
                            VkDeviceSize size,
                            const void* data)
{
    void* dataPtr;
    vkMapMemory(renderDevice->device, memory, offset, size, 0, &dataPtr);
    memcpy(dataPtr, data, size);
    vkUnmapMemory(renderDevice->device, memory);
}

static void copyBuffer(VkCommandBuffer commandBuffer,
                       VkBuffer srcBuffer,
                       VkBuffer dstBuffer,
                       VkDeviceSize srcOffset,
                       VkDeviceSize dstOffset,
                       VkDeviceSize size)
{
    VkBufferCopy copyRegion {
        .srcOffset = srcOffset,
        .dstOffset = dstOffset,
        .size = size
    };

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

VkBufferUsageFlags toVkFlags(BufferType bufferType)
{
    switch (bufferType)
    {
        case BufferType::Vertex: return
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferType::Index: return
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferType::Uniform: return
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferType::Storage: return
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferType::Staging: return
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        default: assert(false);
    }
}

VkMemoryPropertyFlags toVkFlags(MemoryType memoryType)
{
    switch (memoryType)
    {
        case MemoryType::Host: return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        case MemoryType::HostCoherent: return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        case MemoryType::Device: return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        default: assert(false);
    }
}

uint32_t getIndexCount(const VulkanBuffer &buffer)
{
    check(buffer.getBufferType() == BufferType::Index, "Calling getIndexCount() requires the buffer to be an index buffer");
    return buffer.getSize() / 4;
}

VulkanBuffer::VulkanBuffer()
    : mRenderDevice()
    , mBuffer()
    , mMemory()
    , mSize()
{
}

VulkanBuffer::VulkanBuffer(const VulkanRenderDevice &renderDevice,
                           VkDeviceSize size,
                           BufferType bufferType,
                           MemoryType memoryType)
    : mRenderDevice(&renderDevice)
    , mSize(size)
    , mBuffer(createBuffer(bufferType))
    , mMemory(allocateBufferMemory(memoryType, mBuffer))
    , mType(bufferType)
    , mMemoryType(memoryType)
{
}

VulkanBuffer::VulkanBuffer(const VulkanRenderDevice &renderDevice,
                           VkDeviceSize size,
                           BufferType bufferType,
                           MemoryType memoryType,
                           const void *bufferData)
    : mRenderDevice(&renderDevice)
    , mSize(size)
    , mBuffer(createBuffer(bufferType))
    , mMemory(allocateBufferMemory(memoryType, mBuffer))
    , mType(bufferType)
    , mMemoryType(memoryType)
{
    if (memoryType == MemoryType::Host || memoryType == MemoryType::HostCoherent)
    {
        mapBufferMemory(0, size, bufferData);
    }
    else
    {
        VkBuffer stagingBuffer = createBuffer(BufferType::Staging);
        VkDeviceMemory stagingBufferMemory = allocateBufferMemory(MemoryType::Host, stagingBuffer);
        ::mapBufferMemory(mRenderDevice, stagingBufferMemory, 0, size, bufferData);

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(renderDevice);
        ::copyBuffer(commandBuffer, stagingBuffer, mBuffer, 0, 0, size);
        endSingleTimeCommands(renderDevice, commandBuffer);

        vkDestroyBuffer(mRenderDevice->device, stagingBuffer, nullptr);
        vkFreeMemory(mRenderDevice->device, stagingBufferMemory, nullptr);
    }
}

VulkanBuffer::~VulkanBuffer()
{
    if (mRenderDevice)
    {
        vkDestroyBuffer(mRenderDevice->device, mBuffer, nullptr);
        vkFreeMemory(mRenderDevice->device, mMemory, nullptr);
        mSize = 0;
        mRenderDevice = nullptr;
    }
}

VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept
    : VulkanBuffer()
{
    swap(other);
}

VulkanBuffer &VulkanBuffer::operator=(VulkanBuffer &&other) noexcept
{
    if (this != &other)
        swap(other);
    return *this;
}

void VulkanBuffer::mapBufferMemory(VkDeviceSize offset, VkDeviceSize size, const void *data)
{
    ::mapBufferMemory(mRenderDevice, mMemory, offset, size, data);
}

void VulkanBuffer::update(VkDeviceSize offset, VkDeviceSize size, const void *data)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(*mRenderDevice);
    VulkanBuffer stagingBuffer(*mRenderDevice, size, BufferType::Staging, MemoryType::Host, data);
    copyBuffer(commandBuffer, stagingBuffer, 0, offset, size);
    endSingleTimeCommands(*mRenderDevice, commandBuffer);
}

void VulkanBuffer::copyBuffer(VkCommandBuffer commandBuffer, const VulkanBuffer &other, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
{
    ::copyBuffer(commandBuffer, other.mBuffer, mBuffer, srcOffset, dstOffset, size);
}

void VulkanBuffer::copyBuffer(const VulkanBuffer &other, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(*mRenderDevice);
    copyBuffer(commandBuffer, other, srcOffset, dstOffset, size);
    endSingleTimeCommands(*mRenderDevice, commandBuffer);
}

void VulkanBuffer::swap(VulkanBuffer &other) noexcept
{
    std::swap(mRenderDevice, other.mRenderDevice);
    std::swap(mBuffer, other.mBuffer);
    std::swap(mMemory, other.mMemory);
    std::swap(mSize, other.mSize);
    std::swap(mType, other.mType);
    std::swap(mMemoryType, other.mMemoryType);
}

VkBuffer VulkanBuffer::createBuffer(BufferType type)
{
    VkBuffer buffer;

    VkBufferCreateInfo bufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = mSize,
        .usage = toVkFlags(type),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkResult result = vkCreateBuffer(mRenderDevice->device, &bufferCreateInfo, nullptr, &buffer);
    vulkanCheck(result, "Failed to create staging buffer.");

    return buffer;
}

VkDeviceMemory VulkanBuffer::allocateBufferMemory(MemoryType type, VkBuffer buffer)
{
    VkDeviceMemory memory;

    VkMemoryRequirements stagingBufferMemoryRequirements;
    vkGetBufferMemoryRequirements(mRenderDevice->device, buffer, &stagingBufferMemoryRequirements);

    uint32_t memoryTypeIndex = findSuitableMemoryType(mRenderDevice->getMemoryProperties(),
                                                      stagingBufferMemoryRequirements.memoryTypeBits,
                                                      toVkFlags(type)).value();

    VkMemoryAllocateInfo memoryAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = stagingBufferMemoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    VkResult result = vkAllocateMemory(mRenderDevice->device, &memoryAllocateInfo, nullptr, &memory);
    vulkanCheck(result, "Failed to allocate buffer memory.");

    vkBindBufferMemory(mRenderDevice->device, buffer, memory, 0);

    return memory;
}

void VulkanBuffer::setDebugName(const std::string &debugName)
{
    setVulkanObjectDebugName(*mRenderDevice, VK_OBJECT_TYPE_BUFFER, debugName, mBuffer);
    setVulkanObjectDebugName(*mRenderDevice, VK_OBJECT_TYPE_DEVICE_MEMORY, debugName, mMemory);
}

VkDeviceSize VulkanBuffer::getSize() const
{
    return mSize;
}

BufferType VulkanBuffer::getBufferType() const
{
    return mType;
}

MemoryType VulkanBuffer::getMemoryType() const
{
    return mMemoryType;
}

VkBuffer VulkanBuffer::getBuffer() const
{
    return mBuffer;
}

VkDeviceMemory VulkanBuffer::getMemory() const
{
    return mMemory;
}
