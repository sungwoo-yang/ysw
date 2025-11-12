/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "IRenderer2D.hpp"
#include "Shader.hpp"
#include <array>
#include <vector>

class InstancedRenderer2D : public IRenderer2D
{
public:
    InstancedRenderer2D(unsigned max_sprites = 10'000);

    void Init() override;
    void Shutdown() override;
    void BeginScene(std::span<const float, 9> ndc_matrix) override;
    void EndScene() override;
    void DrawQuad(std::span<const float, 9> transform, OpenGL::Handle texture, std::span<const float, 4> texture_coords_lbrt, std::span<const float, 4> tint_color) override;

private:

    struct QuadInstance
    {
        float                        transformrow0[3];
        float                        transformrow1[3];
        std::array<unsigned char, 4> tint{};
        float                        texScale[2];
        float                        texOffset[2];
        int                          textureIndex = 0;
    };

    std::vector<QuadInstance> instanceData{};
    OpenGL::CompiledShader    shader{};
    OpenGL::Handle            fixedVertexBuffer = 0, instanceBuffer = 0, indexBuffer = 0, vertexArrayObject = 0;

    unsigned maxInstances = 0;

    std::vector<OpenGL::Handle> textureSlots;
    size_t                      activeTexuresSize = 0;


private:
    void flush();
    void startBatch();
};