//
// Created by Gianni on 4/02/2025.
//

#ifndef VULKANRENDERINGENGINE_LOADED_IMAGE_HPP
#define VULKANRENDERINGENGINE_LOADED_IMAGE_HPP

#include <glm/glm.hpp>
#include <stb/stb_image.h>

class LoadedImage
{
public:
    LoadedImage();
    LoadedImage(const std::filesystem::path& imagePath);
    ~LoadedImage();

    LoadedImage(const LoadedImage&) = delete;
    LoadedImage& operator=(const LoadedImage&) = delete;

    LoadedImage(LoadedImage&& other) noexcept;
    LoadedImage& operator=(LoadedImage&& other) noexcept;

    void swap(LoadedImage& other);

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


#endif //VULKANRENDERINGENGINE_LOADED_IMAGE_HPP
