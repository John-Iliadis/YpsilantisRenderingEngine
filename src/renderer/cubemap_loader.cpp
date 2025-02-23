//
// Created by Gianni on 24/02/2025.
//

#include "cubemap_loader.hpp"

CubemapLoader::CubemapLoader(const VulkanRenderDevice &renderDevice, const std::array<std::string, 6> &paths)
    : mRenderDevice(renderDevice)
    , mPaths(paths)
    , mDataFutures()
    , mData()
    , mSuccess()
{
    stbi_info(paths.at(0).data(), &mWidth, &mHeight, nullptr);

    if (!validate() || !loadData())
        return;

    createTexture();

    mSuccess = true;
}

bool CubemapLoader::success()
{
    return mSuccess;
}

bool CubemapLoader::validate()
{
    for (uint32_t i = 0; i < 6; ++i)
    {
        int32_t width;
        int32_t height;

        bool validImg = stbi_info(mPaths.at(i).data(), &width, &height, nullptr);

        if (!validImg || mWidth != width || mHeight != height)
            return false;
    }

    return true;
}

bool CubemapLoader::loadData()
{
    for (uint32_t i = 0; i < 6; ++i)
    {
        mDataFutures.at(i) = std::async(std::launch::async, [this, i] () {
            int32_t x, y;
            return stbi_load(mPaths.at(i).data(), &x, &y, nullptr, 4);
        });
    }

    for (uint32_t i = 0; i < 6; ++i)
        mData.at(i) = mDataFutures.at(i).get();

    return std::all_of(mData.begin(), mData.end(), [] (auto p) { return p; });
}

void CubemapLoader::createTexture()
{
    const TextureSpecification specification {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .width = static_cast<uint32_t>(mWidth),
        .height = static_cast<uint32_t>(mHeight),
        .layerCount = 6,
        .imageViewType = VK_IMAGE_VIEW_TYPE_CUBE,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .wrapR = TextureWrap::ClampToEdge,
        .generateMipMaps = false,
        .createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    };

    VkDeviceSize memSize = imageMemoryDeviceSize(mWidth, mHeight, specification.format);

    mCubemapTexture = VulkanTexture(mRenderDevice, specification);

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(mRenderDevice);

    mCubemapTexture.transitionLayout(commandBuffer,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     VK_ACCESS_TRANSFER_WRITE_BIT);

    std::array<VulkanBuffer, 6> stagingBuffers;
    for (uint32_t i = 0; i < 6; ++i)
    {
        stagingBuffers.at(i) = VulkanBuffer(mRenderDevice,
                                            memSize,
                                            BufferType::Staging,
                                            MemoryType::HostCached,
                                            mData.at(i));
        mCubemapTexture.uploadImageData(commandBuffer, stagingBuffers.at(i), i);
    }

    mCubemapTexture.transitionLayout(commandBuffer,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                     VK_ACCESS_TRANSFER_WRITE_BIT,
                                     VK_ACCESS_SHADER_READ_BIT);

    endSingleTimeCommands(mRenderDevice, commandBuffer);
}

CubemapLoader::~CubemapLoader()
{
    for (uint8_t* data : mData)
        stbi_image_free(data);
}

void CubemapLoader::get(VulkanTexture &outTexture)
{
    outTexture.swap(mCubemapTexture);
}
