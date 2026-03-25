/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Texture.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/Image.hpp"
#include "Engine.hpp"
#include "OpenGL/GL.hpp"

namespace CS230
{
    Texture::Texture(const std::filesystem::path& file_name)
    {
        CS200::Image image(file_name, true);
        size          = image.GetSize();
        textureHandle = OpenGL::CreateTextureFromImage(image);
    }

    Texture::Texture(OpenGL::TextureHandle given_texture, Math::ivec2 the_size) : textureHandle(given_texture), size(the_size)
    {
    }

    Texture::~Texture()
    {
        if (textureHandle != 0)
        {
            GL::DeleteTextures(1, &textureHandle);
        }
    }

    Texture::Texture(Texture&& temporary) noexcept : textureHandle(temporary.textureHandle), size(temporary.size)
    {
        temporary.textureHandle = 0;
        temporary.size          = { 0, 0 };
    }

    Texture& Texture::operator=(Texture&& temporary) noexcept
    {
        if (this != &temporary)
        {
            if (textureHandle != 0)
            {
                GL::DeleteTextures(1, &textureHandle);
            }
            textureHandle = temporary.textureHandle;
            size          = temporary.size;

            temporary.textureHandle = 0;
            temporary.size          = { 0, 0 };
        }
        return *this;
    }

    void Texture::Draw(const Math::TransformationMatrix& display_matrix, unsigned int color)
    {
        Math::TransformationMatrix transform =
            display_matrix * Math::TranslationMatrix(Math::vec2{ size.x * 0.5, size.y * 0.5 }) * Math::ScaleMatrix({ static_cast<double>(size.x), static_cast<double>(size.y) });
        Engine::GetRenderer2D().DrawQuad(transform, textureHandle, { 0, 0 }, { 1, 1 }, color);
    }

    void Texture::Draw(const Math::TransformationMatrix& display_matrix, Math::ivec2 texel_position, Math::ivec2 frame_size, unsigned int color)
    {
        const Math::vec2 tex_size = { static_cast<double>(size.x), static_cast<double>(size.y) };

        const Math::vec2 uv_bl = { static_cast<double>(texel_position.x) / tex_size.x, (tex_size.y - static_cast<double>(texel_position.y) - static_cast<double>(frame_size.y)) / tex_size.y };
        const Math::vec2 uv_tr = { (static_cast<double>(texel_position.x) + static_cast<double>(frame_size.x)) / tex_size.x, (tex_size.y - static_cast<double>(texel_position.y)) / tex_size.y };

        Math::TransformationMatrix transform = display_matrix * Math::TranslationMatrix(Math::vec2{ frame_size.x * 0.5, frame_size.y * 0.5 }) *
                                               Math::ScaleMatrix({ static_cast<double>(frame_size.x), static_cast<double>(frame_size.y) });
        Engine::GetRenderer2D().DrawQuad(transform, textureHandle, uv_bl, uv_tr, color);
    }

    Math::ivec2 Texture::GetSize() const
    {
        return size;
    }
}
