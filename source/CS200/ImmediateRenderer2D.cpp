/**
 * \file
 * \author Rudy Castan
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "ImmediateRenderer2D.hpp"

#include "Engine/Matrix.hpp"
#include "Engine/Texture.hpp"
#include "OpenGL/Buffer.hpp"
#include "OpenGL/GL.hpp"
#include "Renderer2DUtils.hpp"
#include <utility>

namespace CS200
{
    ImmediateRenderer2D::ImmediateRenderer2D(ImmediateRenderer2D&& other) noexcept
        : quad(std::exchange(other.quad, {})), quadShader(std::exchange(other.quadShader, {})), sdfQuad(std::exchange(other.sdfQuad, {})), sdfShader(std::exchange(other.sdfShader, {})),
          view_projection(std::exchange(other.view_projection, {}))
    {
    }

    ImmediateRenderer2D& ImmediateRenderer2D::operator=(ImmediateRenderer2D&& other) noexcept
    {
        std::swap(quad, other.quad);
        std::swap(quadShader, other.quadShader);
        std::swap(sdfQuad, other.sdfQuad);
        std::swap(sdfShader, other.sdfShader);
        std::swap(view_projection, other.view_projection);
        return *this;
    }

    ImmediateRenderer2D::~ImmediateRenderer2D()
    {
        Shutdown();
    }

    void ImmediateRenderer2D::Init()
    {
        struct Vertex
        {
            float x, y, u, v;
        };

        constexpr std::array vertices = {
            Vertex{ -0.5f, -0.5f, 0.0f, 0.0f },
            Vertex{ -0.5f,  0.5f, 0.0f, 1.0f },
            Vertex{  0.5f,  0.5f, 1.0f, 1.0f },
            Vertex{  0.5f, -0.5f, 1.0f, 0.0f }
        };
        constexpr std::array<unsigned char, 6> indices = { 0, 1, 2, 0, 2, 3 };

        quad.vertexBuffer = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, std::as_bytes(std::span{ vertices }));
        quad.indexBuffer  = OpenGL::CreateBuffer(OpenGL::BufferType::Indices, std::as_bytes(std::span{ indices }));

        const auto layout = {
            OpenGL::VertexBuffer{ quad.vertexBuffer, { OpenGL::Attribute::Float2, OpenGL::Attribute::Float2 } }
        };
        quad.vertexArray = OpenGL::CreateVertexArrayObject(layout, quad.indexBuffer);

        quadShader = OpenGL::CreateShader(std::filesystem::path{ "Assets/shaders/ImmediateRenderer2D/quad.vert" }, std::filesystem::path{ "Assets/shaders/ImmediateRenderer2D/quad.frag" });

        struct SDFVertex
        {
            float x, y;
        };

        constexpr std::array sdf_vertices = {
            SDFVertex{ -0.5f, -0.5f },
            SDFVertex{ -0.5f,  0.5f },
            SDFVertex{  0.5f,  0.5f },
            SDFVertex{  0.5f, -0.5f }
        };
        sdfQuad.vertexBuffer = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, std::as_bytes(std::span{ sdf_vertices }));
        sdfQuad.indexBuffer  = quad.indexBuffer;

        const auto sdf_layout = {
            OpenGL::VertexBuffer{ sdfQuad.vertexBuffer, { OpenGL::Attribute::Float2 } }
        };
        sdfQuad.vertexArray = OpenGL::CreateVertexArrayObject(sdf_layout, sdfQuad.indexBuffer);

        sdfShader = OpenGL::CreateShader(std::filesystem::path{ "Assets/shaders/ImmediateRenderer2D/sdf.vert" }, std::filesystem::path{ "Assets/shaders/ImmediateRenderer2D/sdf.frag" });
    }

    void ImmediateRenderer2D::Shutdown()
    {
        GL::DeleteBuffers(1, &quad.vertexBuffer);
        GL::DeleteBuffers(1, &quad.indexBuffer);
        GL::DeleteVertexArrays(1, &quad.vertexArray);
        OpenGL::DestroyShader(quadShader);
        quad = {};

        GL::DeleteBuffers(1, &sdfQuad.vertexBuffer);
        GL::DeleteVertexArrays(1, &sdfQuad.vertexArray);
        OpenGL::DestroyShader(sdfShader);
        sdfQuad = {};
    }

    void ImmediateRenderer2D::BeginScene(const Math::TransformationMatrix& view_projection_matrix)
    {
        view_projection          = view_projection_matrix;
        const auto to_ndc_opengl = Renderer2DUtils::to_opengl_mat3(view_projection);

        GL::UseProgram(quadShader.Shader);
        GL::UniformMatrix3fv(quadShader.UniformLocations.at("u_ndc_matrix"), 1, GL_FALSE, to_ndc_opengl.data());

        GL::UseProgram(sdfShader.Shader);
        GL::UniformMatrix3fv(sdfShader.UniformLocations.at("u_ndc_matrix"), 1, GL_FALSE, to_ndc_opengl.data());

        GL::UseProgram(0);
    }

    void ImmediateRenderer2D::EndScene()
    {
    }

    void ImmediateRenderer2D::DrawQuad(const Math::TransformationMatrix& transform, OpenGL::TextureHandle texture, Math::vec2 texture_coord_bl, Math::vec2 texture_coord_tr, CS200::RGBA tintColor)
    {
        GL::UseProgram(quadShader.Shader);
        GL::BindVertexArray(quad.vertexArray);
        GL::BindTexture(GL_TEXTURE_2D, texture);

        const auto& locations    = quadShader.UniformLocations;
        const auto  model_matrix = Renderer2DUtils::to_opengl_mat3(transform);
        GL::UniformMatrix3fv(locations.at("u_model_matrix"), 1, GL_FALSE, model_matrix.data());

        Math::TransformationMatrix uv_transform;
        uv_transform[0][0]   = texture_coord_tr.x - texture_coord_bl.x;
        uv_transform[1][1]   = texture_coord_tr.y - texture_coord_bl.y;
        uv_transform[0][2]   = texture_coord_bl.x;
        uv_transform[1][2]   = texture_coord_bl.y;
        const auto uv_matrix = Renderer2DUtils::to_opengl_mat3(uv_transform);
        GL::UniformMatrix3fv(locations.at("u_uv_matrix"), 1, GL_FALSE, uv_matrix.data());

        const auto color = unpack_color(tintColor);
        GL::Uniform4fv(locations.at("u_tint_color"), 1, color.data());

        GL::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

        GL::BindTexture(GL_TEXTURE_2D, 0);
        GL::BindVertexArray(0);
        GL::UseProgram(0);
    }

    void ImmediateRenderer2D::DrawCircle(const Math::TransformationMatrix& transform, CS200::RGBA fill_color, CS200::RGBA line_color, double line_width)
    {
        DrawSDF(transform, fill_color, line_color, line_width, SDFShape::Circle);
    }

    void ImmediateRenderer2D::DrawRectangle(const Math::TransformationMatrix& transform, CS200::RGBA fill_color, CS200::RGBA line_color, double line_width)
    {
        DrawSDF(transform, fill_color, line_color, line_width, SDFShape::Rectangle);
    }

    void ImmediateRenderer2D::DrawLine(const Math::TransformationMatrix& transform, Math::vec2 start_point, Math::vec2 end_point, CS200::RGBA line_color, double line_width)
    {
        const auto line_transform = Renderer2DUtils::CalculateLineTransform(transform, start_point, end_point, line_width);
        DrawSDF(line_transform, line_color, line_color, line_width, SDFShape::Rectangle);
    }

    void ImmediateRenderer2D::DrawLine(Math::vec2 start_point, Math::vec2 end_point, CS200::RGBA line_color, double line_width)
    {
        DrawLine(Math::TransformationMatrix{}, start_point, end_point, line_color, line_width);
    }

    void ImmediateRenderer2D::DrawSDF(const Math::TransformationMatrix& transform, CS200::RGBA fill_color, CS200::RGBA line_color, double line_width, SDFShape sdf_shape)
    {
        GL::UseProgram(sdfShader.Shader);
        GL::BindVertexArray(sdfQuad.vertexArray);

        const auto& locations = sdfShader.UniformLocations;

        const auto sdf_transform_info = Renderer2DUtils::CalculateSDFTransform(transform, line_width);

        GL::UniformMatrix3fv(locations.at("u_model_matrix"), 1, GL_FALSE, sdf_transform_info.QuadTransform.data());
        GL::Uniform2fv(locations.at("u_world_size"), 1, sdf_transform_info.WorldSize.data());
        GL::Uniform2fv(locations.at("u_quad_size"), 1, sdf_transform_info.QuadSize.data());

        const auto fill = unpack_color(fill_color);
        const auto line = unpack_color(line_color);
        GL::Uniform4fv(locations.at("u_fill_color"), 1, fill.data());
        GL::Uniform4fv(locations.at("u_line_color"), 1, line.data());
        GL::Uniform1f(locations.at("u_line_width"), static_cast<float>(line_width));
        GL::Uniform1i(locations.at("u_shape_type"), static_cast<int>(sdf_shape));

        GL::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

        GL::BindVertexArray(0);
        GL::UseProgram(0);
    }
}
