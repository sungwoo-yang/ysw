/**
 * \file
 * \author Rudy Castan
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "Buffer.hpp"

#include "GL.hpp"

namespace OpenGL
{
    BufferHandle CreateBuffer(BufferType type, GLsizeiptr size_in_bytes) noexcept
    {
        BufferHandle handle = 0;
        GL::GenBuffers(1, &handle);

        GL::BindBuffer(static_cast<GLenum>(type), handle);
        GL::BufferData(static_cast<GLenum>(type), static_cast<GLsizeiptr>(size_in_bytes), nullptr, GL_DYNAMIC_DRAW);
        GL::BindBuffer(static_cast<GLenum>(type), 0);

        return handle;
    }

    BufferHandle CreateBuffer(BufferType type, std::span<const std::byte> static_buffer_data) noexcept
    {
        BufferHandle handle = 0;
        const size_t bytes  = static_buffer_data.size_bytes();
        GL::GenBuffers(1, &handle);

        GL::BindBuffer(static_cast<GLenum>(type), handle);
        GL::BufferData(static_cast<GLenum>(type), static_cast<GLsizeiptr>(bytes), static_buffer_data.data(), GL_STATIC_DRAW);
        GL::BindBuffer(static_cast<GLenum>(type), 0);
        return handle;
    }

    void UpdateBufferData(BufferType type, BufferHandle buffer, std::span<const std::byte> data_to_copy, GLsizei starting_offset) noexcept
    {
        const size_t bytes = data_to_copy.size_bytes();

        GL::BindBuffer(static_cast<GLenum>(type), buffer);
        GL::BufferSubData(static_cast<GLenum>(type), static_cast<GLintptr>(starting_offset), static_cast<GLsizeiptr>(bytes), data_to_copy.data());
        GL::BindBuffer(static_cast<GLenum>(type), 0);
    }
}