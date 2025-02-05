//
// Created by Gianni on 4/02/2025.
//

#ifndef VULKANRENDERINGENGINE_IMAGE_LOADER_HPP
#define VULKANRENDERINGENGINE_IMAGE_LOADER_HPP

#include <glm/glm.hpp>
#include <stb/stb_image.h>

class ImageLoader
{
public:
    ImageLoader();
    ImageLoader(const std::filesystem::path& imagePath, int32_t requiredComponents = 0);
    ~ImageLoader();

    ImageLoader(const ImageLoader&) = delete;
    ImageLoader& operator=(const ImageLoader&) = delete;

    ImageLoader(ImageLoader&& other) noexcept;
    ImageLoader& operator=(ImageLoader&& other) noexcept;

    void swap(ImageLoader& other);

    const std::filesystem::path& path() const;
    bool success() const;
    int32_t width() const;
    int32_t height() const;
    int32_t components() const;
    VkFormat format() const;
    void* data() const;

private:
    std::filesystem::path mPath;
    bool mSuccess;
    int32_t mWidth;
    int32_t mHeight;
    int32_t mComponents;
    VkFormat mFormat;
    void* mData;
};


#endif //VULKANRENDERINGENGINE_IMAGE_LOADER_HPP
