#include "PostProcessor.hpp"
#include "OpenGL/GL.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Window.hpp"
#include <filesystem>

void PostProcessor::Initialize(Math::ivec2 windowSize)
{
    sceneFBO = OpenGL::CreateFramebufferWithColor(windowSize);
    bloomMaskFBO = OpenGL::CreateFramebufferWithColor(windowSize);

    shader = OpenGL::CreateShader(
        std::filesystem::path("Assets/shaders/PostProcess.vert"),
        std::filesystem::path("Assets/shaders/Bloom.frag")
    );

    std::vector<float> quadVertices = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

    quadVBO = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, std::as_bytes(std::span{ quadVertices }));
    OpenGL::VertexBuffer vb;
    vb.Handle = quadVBO;
    vb.Layout = OpenGL::BufferLayout({ OpenGL::Attribute::Float2, OpenGL::Attribute::Float2 });
    quadVAO = OpenGL::CreateVertexArrayObject({ vb });

}

void PostProcessor::BeginSceneRender()
{
    GL::BindFramebuffer(GL_FRAMEBUFFER, sceneFBO.Framebuffer);
    Engine::GetWindow().Clear(0x000000FF);
}

void PostProcessor::BeginBloomMaskRender()
{
    GL::BindFramebuffer(GL_FRAMEBUFFER, bloomMaskFBO.Framebuffer);
    Engine::GetWindow().Clear(0x000000FF);
}

void PostProcessor::EndRenderAndDraw() 
{
    GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
    Engine::GetWindow().Clear(0x000000FF);

    GL::UseProgram(shader.Shader);

    GL::ActiveTexture(GL_TEXTURE0);
    GL::BindTexture(GL_TEXTURE_2D, sceneFBO.ColorAttachment);
    GLint loc0 = GL::GetUniformLocation(shader.Shader, "u_SceneTexture");
    if (loc0 != -1) GL::Uniform1i(loc0, 0);

    GL::ActiveTexture(GL_TEXTURE1);
    GL::BindTexture(GL_TEXTURE_2D, bloomMaskFBO.ColorAttachment);
    GLint loc1 = GL::GetUniformLocation(shader.Shader, "u_BloomMaskTexture");
    if (loc1 != -1) GL::Uniform1i(loc1, 1);

    GL::BindVertexArray(quadVAO);
    GL::DrawArrays(GL_TRIANGLE_FAN, 0, 4);

    GL::BindVertexArray(0);
    GL::UseProgram(0);

    GL::ActiveTexture(GL_TEXTURE0);
    GL::BindTexture(GL_TEXTURE_2D, 0);

}

void PostProcessor::Shutdown()
{
    OpenGL::DestroyFramebufferWithColor(sceneFBO);
    OpenGL::DestroyFramebufferWithColor(bloomMaskFBO);
    OpenGL::DestroyShader(shader);
    GL::DeleteVertexArrays(1, &quadVAO);
    GL::DeleteBuffers(1, &quadVBO);
}