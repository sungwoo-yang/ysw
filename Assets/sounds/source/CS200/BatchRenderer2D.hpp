/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine/Matrix.hpp"
#include "IRenderer2D.hpp"
#include "OpenGL/Shader.hpp"
#include "OpenGL/VertexArray.hpp"
#include <array>
#include <vector>

namespace CS200
{
    class BatchRenderer2D : public IRenderer2D
    {
    public:
        BatchRenderer2D();
        ~BatchRenderer2D() override;

        BatchRenderer2D(const BatchRenderer2D& other)                = delete;
        BatchRenderer2D(BatchRenderer2D&& other) noexcept            = delete;
        BatchRenderer2D& operator=(const BatchRenderer2D& other)     = delete;
        BatchRenderer2D& operator=(BatchRenderer2D&& other) noexcept = delete;

        void Init() override;
        void Shutdown() override;
        void BeginScene(const Math::TransformationMatrix& view_projection) override;
        void EndScene() override;
        void DrawQuad(const Math::TransformationMatrix& transform, OpenGL::TextureHandle texture, Math::vec2 texture_coord_bl, Math::vec2 texture_coord_tr, CS200::RGBA tintColor) override;

        void DrawCircle(const Math::TransformationMatrix& transform, CS200::RGBA fill_color, CS200::RGBA line_color, double line_width) override;
        void DrawRectangle(const Math::TransformationMatrix& transform, CS200::RGBA fill_color, CS200::RGBA line_color, double line_width) override;
        void DrawLine(const Math::TransformationMatrix& transform, Math::vec2 start_point, Math::vec2 end_point, CS200::RGBA line_color, double line_width) override;
        void DrawLine(Math::vec2 start_point, Math::vec2 end_point, CS200::RGBA line_color, double line_width) override;

    private:
        void  Flush();
        void  StartBatch();
        float GetTextureSlot(OpenGL::TextureHandle texture);

        struct QuadVertex
        {
            std::array<float, 2> Position;
            std::array<float, 4> TintColor;
            std::array<float, 2> TexCoord;
            float                TexID;
        };

        OpenGL::VertexArrayHandle m_VAO = 0;
        OpenGL::BufferHandle      m_VBO = 0;
        OpenGL::BufferHandle      m_EBO = 0;
        OpenGL::CompiledShader    m_Shader{};

        std::vector<QuadVertex> m_Vertices;
        uint32_t                m_IndexCount = 0;

        std::vector<OpenGL::TextureHandle> m_TextureSlots;
        uint32_t                           m_TextureSlotIndex = 1;

        static constexpr uint32_t MAX_QUADS_PER_BATCH    = 10000;
        static constexpr uint32_t MAX_VERTICES_PER_BATCH = MAX_QUADS_PER_BATCH * 4;
        static constexpr uint32_t MAX_INDICES_PER_BATCH  = MAX_QUADS_PER_BATCH * 6;

        Math::TransformationMatrix m_ViewProjection{};

        static const std::array<std::array<float, 2>, 4> s_UnitQuadPositions;
    };
}