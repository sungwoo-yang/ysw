/**
 * \file
 * \author Rudy Castan
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "Image.hpp"

#include "Engine/Error.hpp"
#include "Engine/Path.hpp"

#include <stb_image.h>

namespace CS200
{
    Image::Image(const std::filesystem::path& image_path, bool flip_vertical)
    {
        stbi_set_flip_vertically_on_load(flip_vertical);
        int               width, height, channels;
        const std::string path_string = assets::locate_asset(image_path).string();

        unsigned char* raw_pixels = stbi_load(path_string.c_str(), &width, &height, &channels, 4);

        if (raw_pixels == nullptr)
        {
            throw_error_message("Failed to load image: " + path_string + " Reason: " + stbi_failure_reason());
        }

        pixels = reinterpret_cast<RGBA*>(raw_pixels);
        size   = { width, height };
    }

    Image::~Image()
    {
        if (pixels)
        {
            stbi_image_free(pixels);
            pixels = nullptr;
        }
    }

    Image::Image(Image&& temporary) noexcept : pixels(temporary.pixels), size(temporary.size)
    {
        temporary.pixels = nullptr;
        temporary.size   = { 0, 0 };
    }

    Image& Image::operator=(Image&& temporary) noexcept
    {
        if (this != &temporary)
        {
            if (pixels)
            {
                stbi_image_free(pixels);
            }
            pixels           = temporary.pixels;
            size             = temporary.size;
            temporary.pixels = nullptr;
            temporary.size   = { 0, 0 };
        }
        return *this;
    }

    const RGBA* Image::data() const noexcept
    {
        return pixels;
    }

    RGBA* Image::data() noexcept
    {
        return pixels;
    }

    Math::ivec2 Image::GetSize() const noexcept
    {
        return size;
    }
}
