#pragma once
#include "OpenGL/Framebuffer.hpp"
#include "OpenGL/Shader.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/Buffer.hpp"
#include "Engine/Vec2.hpp"

class PostProcessor {
public:
    void Initialize(Math::ivec2 windowSize);

    void BeginSceneRender();     
    void BeginBloomMaskRender();  
     
    void Shutdown();

    void EndRenderAndDraw();
private:
    OpenGL::FramebufferWithColor sceneFBO;
    OpenGL::FramebufferWithColor bloomMaskFBO; 
    OpenGL::CompiledShader       shader;
    OpenGL::VertexArrayHandle    quadVAO;
    OpenGL::BufferHandle         quadVBO;
};