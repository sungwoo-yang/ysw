/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "TextureManager.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine.hpp"
#include "Logger.hpp"
#include "OpenGL/Framebuffer.hpp"
#include "OpenGL/GL.hpp"
#include "Texture.hpp"
#include <algorithm>

std::shared_ptr<CS230::Texture> CS230::TextureManager::Load(const std::filesystem::path& file_name)
{
    const std::string path_string = file_name.string();
    if (textures.count(path_string))
    {
        return textures.at(path_string);
    }

    auto new_texture      = std::shared_ptr<Texture>(new Texture(file_name));
    textures[path_string] = new_texture;
    Engine::GetLogger().LogDebug("Loading Texture: " + path_string);
    return new_texture;
}

void CS230::TextureManager::Unload()
{
    textures.clear();
    Engine::GetLogger().LogEvent("Clearing Textures");
}

namespace
{
    struct RenderState
    {
        OpenGL::FramebufferWithColor framebuffer{};
        Math::ivec2                  size{};
        std::array<GLfloat, 4>       clearColor{};
        std::array<GLint, 4>         viewport{};
    };

    RenderState savedState;
}

void CS230::TextureManager::StartRenderTextureMode(int width, int height)
{
    auto& renderer = Engine::GetRenderer2D();
    renderer.EndScene();

    savedState.framebuffer = OpenGL::CreateFramebufferWithColor({ width, height });
    GL::GetFloatv(GL_COLOR_CLEAR_VALUE, savedState.clearColor.data());
    GL::GetIntegerv(GL_VIEWPORT, savedState.viewport.data());

    savedState.size = { width, height };

    GL::BindFramebuffer(GL_FRAMEBUFFER, savedState.framebuffer.Framebuffer);
    GL::Viewport(0, 0, width, height);
    GL::ClearColor(0.f, 0.f, 0.f, 0.f);
    GL::Clear(GL_COLOR_BUFFER_BIT);

    Math::TransformationMatrix ndc_matrix = CS200::build_ndc_matrix({ width, height });
    renderer.BeginScene(ndc_matrix);
}

std::shared_ptr<CS230::Texture> CS230::TextureManager::EndRenderTextureMode()
{
    auto& renderer = Engine::GetRenderer2D();
    renderer.EndScene();

    GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
    GL::Viewport(savedState.viewport[0], savedState.viewport[1], savedState.viewport[2], savedState.viewport[3]);
    GL::ClearColor(savedState.clearColor[0], savedState.clearColor[1], savedState.clearColor[2], savedState.clearColor[3]);

    auto new_texture = std::shared_ptr<Texture>(new Texture(savedState.framebuffer.ColorAttachment, savedState.size));

    auto temp_fbo                          = savedState.framebuffer.Framebuffer;
    savedState.framebuffer.ColorAttachment = 0;
    savedState.framebuffer.Framebuffer     = 0;
    GL::DeleteFramebuffers(1, &temp_fbo);

    renderer.BeginScene(CS200::build_ndc_matrix({ savedState.viewport[2], savedState.viewport[3] }));

    return new_texture;
}
