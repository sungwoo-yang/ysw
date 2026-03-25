/**
 * \file
 * \author Rudy Castan
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "BatchRenderer2D.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Path.hpp"
#include "Engine/Texture.hpp"
#include "OpenGL/Buffer.hpp"
#include "OpenGL/GL.hpp"
#include "Renderer2DUtils.hpp"
#include <fstream>
#include <numeric>
#include <sstream>
#include <utility>

namespace CS200
{
    const std::array<std::array<float, 2>, 4> BatchRenderer2D::s_UnitQuadPositions = {
        std::array<float, 2>{ -0.5f, -0.5f },
         std::array<float, 2>{  0.5f, -0.5f },
         std::array<float, 2>{  0.5f,  0.5f },
         std::array<float, 2>{ -0.5f,  0.5f }
    };

    BatchRenderer2D::BatchRenderer2D()
    {
        m_Vertices.reserve(MAX_VERTICES_PER_BATCH);
    }

    BatchRenderer2D::~BatchRenderer2D()
    {
        if (m_VAO != 0)
        {
            Shutdown();
        }
    }

    void BatchRenderer2D::Init()
    {
        GLint max_tex_units = 0;
        GL::GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_tex_units);
        m_TextureSlots.resize(static_cast<size_t>(std::min(max_tex_units, 32)));

        const std::filesystem::path vertex_file = assets::locate_asset("Assets/shaders/BatchRenderer2D/batch.vert");
        std::ifstream               vert_stream(vertex_file);
        std::stringstream           vert_text_stream;
        vert_text_stream << vert_stream.rdbuf();
        const std::string vertex_glsl = vert_text_stream.str();

        const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/BatchRenderer2D/batch.frag");
        std::ifstream               frag_stream(fragment_file);
        std::stringstream           frag_text_stream;
        frag_text_stream << frag_stream.rdbuf();
        std::string frag_glsl = frag_text_stream.str();

        const size_t first_newline = frag_glsl.find('\n');
        if (first_newline != std::string::npos)
        {
            const std::string define_line = "\n#define MAX_TEXTURE_SLOTS " + std::to_string(m_TextureSlots.size()) + "\n";
            frag_glsl.insert(first_newline + 1, define_line);
        }

        m_Shader = OpenGL::CreateShader(std::string_view{ vertex_glsl }, std::string_view{ frag_glsl });
        m_VBO    = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, MAX_VERTICES_PER_BATCH * sizeof(QuadVertex));

        std::vector<uint32_t> indices(MAX_INDICES_PER_BATCH);
        uint32_t              offset = 0;

        for (size_t i = 0; i < MAX_INDICES_PER_BATCH; i += 6)
        {
            indices[i + 0] = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 0;
            indices[i + 4] = offset + 2;
            indices[i + 5] = offset + 3;

            offset += 4;
        }

        m_EBO = OpenGL::CreateBuffer(OpenGL::BufferType::Indices, std::as_bytes(std::span{ indices }));

        OpenGL::VertexBuffer vboLayout{
            m_VBO, { OpenGL::Attribute::Float2, OpenGL::Attribute::Float4, OpenGL::Attribute::Float2, OpenGL::Attribute::Float }
        };
        m_VAO = OpenGL::CreateVertexArrayObject(vboLayout, m_EBO);

        m_TextureSlots[0] = 0;

        std::vector<int> samplers(m_TextureSlots.size());
        std::iota(samplers.begin(), samplers.end(), 0);

        GL::UseProgram(m_Shader.Shader);
        GL::Uniform1iv(m_Shader.UniformLocations.at("u_textures[0]"), static_cast<GLsizei>(m_TextureSlots.size()), samplers.data());
        GL::UseProgram(0);
    }

    void BatchRenderer2D::Shutdown()
    {
        GL::DeleteBuffers(1, &m_VBO);
        GL::DeleteBuffers(1, &m_EBO);
        GL::DeleteVertexArrays(1, &m_VAO);
        OpenGL::DestroyShader(m_Shader);

        m_VBO           = 0;
        m_EBO           = 0;
        m_VAO           = 0;
        m_Shader.Shader = 0;
    }

    void BatchRenderer2D::BeginScene(const Math::TransformationMatrix& view_projection)
    {
        m_drawCallCount  = 0;
        m_ViewProjection = view_projection;
        StartBatch();
    }

    void BatchRenderer2D::EndScene()
    {
        Flush();
    }

    void BatchRenderer2D::Flush()
    {
        if (m_Vertices.empty())
        {
            return;
        }

        GL::UseProgram(m_Shader.Shader);
        GL::BindVertexArray(m_VAO);

        OpenGL::UpdateBufferData(OpenGL::BufferType::Vertices, m_VBO, std::as_bytes(std::span{ m_Vertices }));

        for (uint32_t i = 0; i < m_TextureSlotIndex; ++i)
        {
            GL::ActiveTexture(GL_TEXTURE0 + i);
            GL::BindTexture(GL_TEXTURE_2D, m_TextureSlots[i]);
        }

        const auto gl_vp_matrix = Renderer2DUtils::to_opengl_mat3(m_ViewProjection);
        GL::UniformMatrix3fv(m_Shader.UniformLocations.at("u_ndc_matrix"), 1, GL_FALSE, gl_vp_matrix.data());

        GL::DrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndexCount), GL_UNSIGNED_INT, nullptr);
        m_drawCallCount++;

        GL::BindVertexArray(0);
        GL::UseProgram(0);
    }

    void BatchRenderer2D::StartBatch()
    {
        m_Vertices.clear();
        m_IndexCount       = 0;
        m_TextureSlotIndex = 0;
    }

    float BatchRenderer2D::GetTextureSlot(OpenGL::TextureHandle texture)
    {
        for (uint32_t i = 0; i < m_TextureSlotIndex; ++i)
        {
            if (m_TextureSlots[i] == texture)
            {
                return static_cast<float>(i);
            }
        }

        if (m_TextureSlotIndex >= m_TextureSlots.size())
        {
            Flush();
            StartBatch();
        }

        m_TextureSlots[m_TextureSlotIndex] = texture;

        return static_cast<float>(m_TextureSlotIndex++);
    }

    void BatchRenderer2D::DrawQuad(const Math::TransformationMatrix& transform, OpenGL::TextureHandle texture, Math::vec2 texture_coord_bl, Math::vec2 texture_coord_tr, CS200::RGBA tintColor)
    {
        if (m_Vertices.size() + 4 > MAX_VERTICES_PER_BATCH)
        {
            Flush();
            StartBatch();
        }

        const float texID = GetTextureSlot(texture);
        const auto  color = CS200::unpack_color(tintColor);

        const std::array<std::array<float, 2>, 4> texCoords = {
            { { static_cast<float>(texture_coord_bl.x), static_cast<float>(texture_coord_bl.y) },
             { static_cast<float>(texture_coord_tr.x), static_cast<float>(texture_coord_bl.y) },
             { static_cast<float>(texture_coord_tr.x), static_cast<float>(texture_coord_tr.y) },
             { static_cast<float>(texture_coord_bl.x), static_cast<float>(texture_coord_tr.y) } }
        };

        for (size_t i = 0; i < 4; ++i)
        {
            QuadVertex vertex;
            Math::vec2 local_pos = { static_cast<double>(s_UnitQuadPositions[i][0]), static_cast<double>(s_UnitQuadPositions[i][1]) };
            Math::vec2 world_pos = transform * local_pos;
            vertex.Position      = { static_cast<float>(world_pos.x), static_cast<float>(world_pos.y) };
            vertex.TintColor     = color;
            vertex.TexCoord      = texCoords[i];
            vertex.TexID         = texID;
            m_Vertices.push_back(vertex);
        }

        m_IndexCount += 6;
    }

    void BatchRenderer2D::DrawCircle(const Math::TransformationMatrix&, CS200::RGBA, CS200::RGBA, double)
    {
    }

    void BatchRenderer2D::DrawRectangle(const Math::TransformationMatrix&, CS200::RGBA, CS200::RGBA, double)
    {
    }

    void BatchRenderer2D::DrawLine(const Math::TransformationMatrix&, Math::vec2, Math::vec2, CS200::RGBA, double)
    {
    }

    void BatchRenderer2D::DrawLine(Math::vec2, Math::vec2, CS200::RGBA, double)
    {
    }
}