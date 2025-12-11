/**
 * \file
 * \author Rudy Castan
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "RenderingAPI.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Error.hpp"
#include "Engine/Logger.hpp"
#include "OpenGL/Environment.hpp"
#include "OpenGL/GL.hpp"
#include <GL/glew.h>
#include <cassert>

namespace
{
#if defined(DEVELOPER_VERSION) && not defined(IS_WEBGL2)
    void OpenGLMessageCallback(
        [[maybe_unused]] unsigned source, [[maybe_unused]] unsigned type, [[maybe_unused]] unsigned id, unsigned severity, [[maybe_unused]] int length, const char* message,
        [[maybe_unused]] const void* userParam)
    {
        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH: Engine::GetLogger().LogError(message); return;
            case GL_DEBUG_SEVERITY_MEDIUM: Engine::GetLogger().LogError(message); return;
            case GL_DEBUG_SEVERITY_LOW: Engine::GetLogger().LogVerbose(message); return;
            case GL_DEBUG_SEVERITY_NOTIFICATION: Engine::GetLogger().LogVerbose(message); return;
        }

        assert(false && "Unknown severity level!");
    }
#endif
}

namespace CS200::RenderingAPI
{
    void Init() noexcept
    {
        GLint major = 0, minor = 0;
        GL::GetIntegerv(GL_MAJOR_VERSION, &major);
        GL::GetIntegerv(GL_MINOR_VERSION, &minor);
        if (OpenGL::version(major, minor) < OpenGL::version(OpenGL::MinimumRequiredMajorVersion, OpenGL::MinimumRequiredMinorVersion))
            throw_error_message("Unsupported OpenGL version ", major, '.', minor, "\n We need OpenGL ", OpenGL::MinimumRequiredMajorVersion, '.', OpenGL::MinimumRequiredMinorVersion, " or higher");

        if (OpenGL::MajorVersion == 0)
        {
            OpenGL::MajorVersion = major;
            OpenGL::MinorVersion = minor;
        }

        GL::GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &OpenGL::MaxTextureImageUnits);
        GL::GetIntegerv(GL_MAX_TEXTURE_SIZE, &OpenGL::MaxTextureSize);

#if defined(DEVELOPER_VERSION) && not defined(IS_WEBGL2)
        // Debug callback functionality requires OpenGL 4.3+ or KHR_debug extension
        if (OpenGL::current_version() >= OpenGL::version(4, 3))
        {
            GL::Enable(GL_DEBUG_OUTPUT);
            GL::Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            GL::DebugMessageCallback(OpenGLMessageCallback, nullptr);
            GL::DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        }
#endif

        GL::Enable(GL_BLEND);
        GL::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL::Disable(GL_DEPTH_TEST);

        auto& logger = Engine::GetLogger();
        logger.LogDebug("---------- OpenGL Setting ----------");

        const auto get_string = [](GLenum name)
        {
            return reinterpret_cast<const char*>(GL::GetString(name));
        };
        logger.LogDebug("Vendor: " + std::string(get_string(GL_VENDOR)));
        logger.LogDebug("Renderer: " + std::string(get_string(GL_RENDERER)));
        logger.LogDebug("Version: " + std::string(get_string(GL_VERSION)));
        logger.LogDebug("GLSL Version: " + std::string(get_string(GL_SHADING_LANGUAGE_VERSION)));
        logger.LogDebug("Major Version: " + std::to_string(major));
        logger.LogDebug("Minor Version: " + std::to_string(minor));

        GLint max_verts = 0, max_indices = 0;
        GL::GetIntegerv(GL_MAX_ELEMENTS_VERTICES, &max_verts);
        GL::GetIntegerv(GL_MAX_ELEMENTS_INDICES, &max_indices);
        logger.LogDebug("Max elements vertices: " + std::to_string(max_verts));
        logger.LogDebug("Max elements indices: " + std::to_string(max_indices));

        GLint max_viewport_dims[2];
        GL::GetIntegerv(GL_MAX_VIEWPORT_DIMS, max_viewport_dims);
        logger.LogDebug("Max texture image units: " + std::to_string(OpenGL::MaxTextureImageUnits));
        logger.LogDebug("Max texture size: " + std::to_string(OpenGL::MaxTextureSize));
        logger.LogDebug("Max viewport dims: " + std::to_string(max_viewport_dims[0]) + " x " + std::to_string(max_viewport_dims[1]));
        logger.LogDebug("-----------------------------------");
    }

    void SetClearColor(CS200::RGBA color) noexcept
    {
        const auto rgba = CS200::unpack_color(color);
        GL::ClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    void Clear() noexcept
    {
        GL::Clear(GL_COLOR_BUFFER_BIT);
    }

    void SetViewport(Math::ivec2 size, Math::ivec2 anchor_left_bottom) noexcept
    {
        GL::Viewport(anchor_left_bottom.x, anchor_left_bottom.y, size.x, size.y);
    }
}