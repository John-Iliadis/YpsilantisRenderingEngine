//
// Created by Gianni on 4/02/2025.
//

#include "image_loader.hpp"

ImageLoader::ImageLoader()
    : mPath()
    , mSuccess()
    , mWidth()
    , mHeight()
    , mComponents()
    , mFormat()
    , mData()
{
}

ImageLoader::ImageLoader(const std::filesystem::path &imagePath, int32_t requiredComponents)
    : ImageLoader()
{
    mPath = imagePath;
    bool isHDR = stbi_is_hdr(imagePath.string().c_str());

    if (isHDR)
    {
        mData = stbi_loadf(imagePath.string().c_str(), &mWidth, &mHeight, &mComponents, requiredComponents);
    }
    else
    {
        mData = stbi_load(imagePath.string().c_str(), &mWidth, &mHeight, &mComponents, requiredComponents);
    }

    if (!mData)
        return;

    switch (mComponents)
    {
        case 1: mFormat = isHDR? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R8_UNORM; break;
        case 2: mFormat = isHDR? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R8G8_UNORM; break;
        case 3: mFormat = isHDR? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R8G8B8_UNORM; break;
        case 4: mFormat = isHDR? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM; break;
        default: assert(false);
    }

    mSuccess = true;
}

ImageLoader::~ImageLoader() { stbi_image_free(mData); }

ImageLoader::ImageLoader(ImageLoader &&other) noexcept
    : ImageLoader()
{
    swap(other);
}

ImageLoader &ImageLoader::operator=(ImageLoader &&other) noexcept
{
    if (this != &other)
    {
        mPath = "";
        mSuccess = false;
        mWidth = 0;
        mHeight = 0;
        mComponents = 0;
        mFormat = VK_FORMAT_UNDEFINED;
        mData = nullptr;

        swap(other);
    }

    return *this;
}

void ImageLoader::swap(ImageLoader &other)
{
    std::swap(mPath, other.mPath);
    std::swap(mSuccess, other.mSuccess);
    std::swap(mWidth, other.mWidth);
    std::swap(mHeight, other.mHeight);
    std::swap(mComponents, other.mComponents);
    std::swap(mFormat, other.mFormat);
    std::swap(mData, other.mData);
}

const std::filesystem::path &ImageLoader::path() const
{
    return mPath;
}

bool ImageLoader::success() const
{
    return mSuccess;
}

int32_t ImageLoader::width() const
{
    return mWidth;
}

int32_t ImageLoader::height() const
{
    return mHeight;
}

int32_t ImageLoader::components() const
{
    return mComponents;
}

VkFormat ImageLoader::format() const
{
    return mFormat;
}

void *ImageLoader::data() const
{
    return mData;
}