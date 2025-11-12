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

class BatchRenderer2D : public IRenderer2D
{
public:
    BatchRenderer2D(unsigned max_quads = 10'000);

    void Init() override;
    void Shutdown() override;
    void BeginScene(std::span<const float, 9> ndc_matrix) override;
    void EndScene() override;
    void DrawQuad(std::span<const float, 9> transform, OpenGL::Handle texture, std::span<const float, 4> texture_coords_lbrt, std::span<const float, 4> tint_color) override;

private:

    struct QuadVertex
    {
        float                        x = 0, y = 0;
        float                        s = 0, t = 0;
        std::array<unsigned char, 4> tint{};
        int                          textureIndex = 0;
    };

    std::vector<QuadVertex> vertexData{};
    OpenGL::CompiledShader  shader{};
    OpenGL::Handle          vertexBuffer = 0, indexBuffer = 0, vertexArrayObject = 0;

    unsigned maxVertices = 0;
    unsigned maxIndices  = 0;

    QuadVertex* vertexDataEnd = nullptr;
    unsigned    indexCount    = 0;

    std::vector<OpenGL::Handle> textureSlots;
    size_t                      activeTexuresSize = 0;


private:
    void flush();
    void startBatch();
};