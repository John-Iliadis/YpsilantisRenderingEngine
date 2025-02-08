//
// Created by Gianni on 4/02/2025.
//

#include "loaded_image.hpp"

LoadedImage::LoadedImage()
    : mPath()
    , mSuccess()
    , mWidth()
    , mHeight()
    , mComponents()
    , mFormat()
    , mData()
{
}

LoadedImage::LoadedImage(const std::filesystem::path &imagePath)
    : LoadedImage()
{
    mPath = imagePath;
    bool isHDR = stbi_is_hdr(imagePath.string().c_str());

    if (isHDR)
    {
        mData = stbi_loadf(imagePath.string().c_str(), &mWidth, &mHeight, &mComponents, 4);
    }
    else
    {
        mData = stbi_load(imagePath.string().c_str(), &mWidth, &mHeight, &mComponents, 4);
    }

    if (!mData)
        return;

    mFormat = isHDR? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;
    mSuccess = true;
}

LoadedImage::~LoadedImage() { stbi_image_free(mData); }

LoadedImage::LoadedImage(LoadedImage &&other) noexcept
    : LoadedImage()
{
    swap(other);
}

LoadedImage &LoadedImage::operator=(LoadedImage &&other) noexcept
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

void LoadedImage::swap(LoadedImage &other)
{
    std::swap(mPath, other.mPath);
    std::swap(mSuccess, other.mSuccess);
    std::swap(mWidth, other.mWidth);
    std::swap(mHeight, other.mHeight);
    std::swap(mComponents, other.mComponents);
    std::swap(mFormat, other.mFormat);
    std::swap(mData, other.mData);
}

const std::filesystem::path &LoadedImage::path() const
{
    return mPath;
}

bool LoadedImage::success() const
{
    return mSuccess;
}

int32_t LoadedImage::width() const
{
    return mWidth;
}

int32_t LoadedImage::height() const
{
    return mHeight;
}

int32_t LoadedImage::components() const
{
    return mComponents;
}

VkFormat LoadedImage::format() const
{
    return mFormat;
}

void *LoadedImage::data() const
{
    return mData;
}