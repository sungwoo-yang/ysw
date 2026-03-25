/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "IRenderer2D.hpp"
#include "OpenGL/Buffer.hpp"
#include "OpenGL/Shader.hpp"
#include "OpenGL/VertexArray.hpp"
#include "Renderer2DUtils.hpp"
#include <array>
#include <vector>

namespace CS200
{
    class InstancedRenderer2D : public IRenderer2D
    {
    public:
        InstancedRenderer2D();
        ~InstancedRenderer2D();

        InstancedRenderer2D(const InstancedRenderer2D&)                = delete;
        InstancedRenderer2D(InstancedRenderer2D&&) noexcept            = delete;
        InstancedRenderer2D& operator=(const InstancedRenderer2D&)     = delete;
        InstancedRenderer2D& operator=(InstancedRenderer2D&&) noexcept = delete;
        void                 Init() override;
        void                 Shutdown() override;

        void BeginScene(const Math::TransformationMatrix& view_projection) override;
        void EndScene() override;

        void DrawQuad(const Math::TransformationMatrix& transform, OpenGL::TextureHandle texture, Math::vec2 texture_coord_bl, Math::vec2 texture_coord_tr, CS200::RGBA tintColor) override;

        void DrawCircle(const Math::TransformationMatrix& transform, CS200::RGBA fill_color, CS200::RGBA line_color, double line_width) override;
        void DrawRectangle(const Math::TransformationMatrix& transform, CS200::RGBA fill_color, CS200::RGBA line_color, double line_width) override;
        void DrawLine(const Math::TransformationMatrix& transform, Math::vec2 start_point, Math::vec2 end_point, CS200::RGBA line_color, double line_width) override;
        void DrawLine(Math::vec2 start_point, Math::vec2 end_point, CS200::RGBA line_color, double line_width) override;

    private:
        void Flush();
        void StartBatch();
        int  GetTextureSlot(OpenGL::TextureHandle texture);

        struct InstanceData
        {
            std::array<float, 3> ModelRow0;
            std::array<float, 3> ModelRow1;
            std::array<float, 4> TintColor;
            std::array<float, 2> TexCoordScale;
            std::array<float, 2> TexCoordOffset;
            int                  TexID;
        };

        OpenGL::VertexArrayHandle m_VAO         = 0;
        OpenGL::BufferHandle      m_UnitQuadVBO = 0;
        OpenGL::BufferHandle      m_InstanceVBO = 0;
        OpenGL::BufferHandle      m_EBO         = 0;
        OpenGL::CompiledShader    m_Shader{};

        std::vector<InstanceData> m_InstanceData;
        uint32_t                  m_InstanceCount = 0;

        std::vector<OpenGL::TextureHandle> m_TextureSlots;
        uint32_t                           m_TextureSlotIndex = 1;

        static constexpr uint32_t MAX_INSTANCES_PER_BATCH = 10000;

        Math::TransformationMatrix m_ViewProjection{};
    };
}
