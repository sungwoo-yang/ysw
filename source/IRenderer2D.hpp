/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Handle.hpp"
#include <span>

class IRenderer2D
{
public:
    virtual ~IRenderer2D()                                                  = default;
    virtual void           Init()                                           = 0;
    virtual void           Shutdown()                                       = 0;
    virtual void           BeginScene(std::span<const float, 9> ndc_matrix) = 0;
    virtual void           EndScene()                                       = 0;
    constexpr static float WHITE[]                                          = { 1.0f, 1.0f, 1.0f, 1.0f };
    constexpr static float WHOLE_IMAGE[]                                    = { 0.0f, 0.0f, 1.0f, 1.0f };
    virtual void DrawQuad(std::span<const float, 9> transform, OpenGL::Handle texture, std::span<const float, 4> texture_coords_lbrt = WHOLE_IMAGE, std::span<const float, 4> tint_color = WHITE) = 0;
};
