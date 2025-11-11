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

/**
 * \brief Immediate mode renderer - draws each quad with individual draw calls
 */
class ImmediateRenderer2D : public IRenderer2D
{
public:
    void Init() override;
    void Shutdown() override;
    void BeginScene(std::span<const float, 9> ndc_matrix) override;
    void EndScene() override;
    void DrawQuad(std::span<const float, 9> transform, OpenGL::Handle texture, std::span<const float, 4> texture_coords_lbrt, std::span<const float, 4> tint_color) override;

private:
    OpenGL::CompiledShader shader;
    OpenGL::Handle         vertex_buffer;
    OpenGL::Handle         index_buffer;
    OpenGL::Handle         vertex_array_object;
    GLsizei                indices_count = 0;
};
