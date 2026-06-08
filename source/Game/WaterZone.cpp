#include "WaterZone.hpp"
#include "Engine/Engine.hpp"
#include "OpenGL/Shader.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>

// ── Shared resources ─────────────────────────────────────────────────────────
OpenGL::CompiledShader WaterZone::s_shader   = {};
int                    WaterZone::s_refCount = 0;

// ── Lifecycle ─────────────────────────────────────────────────────────────────

WaterZone::WaterZone(Math::vec2 pos, Math::vec2 size)
    : CS230::GameObject(pos), size(size)
{
    EnsureGL();
}

WaterZone::~WaterZone()
{
    if (_vao) { GL::DeleteVertexArrays(1, &_vao); _vao = 0; }
    if (_vbo) { GL::DeleteBuffers(1, &_vbo);      _vbo = 0; }

    if (--s_refCount == 0)
        OpenGL::DestroyShader(s_shader);
}

void WaterZone::EnsureGL()
{
    if (s_refCount == 0)
    {
        namespace fs = std::filesystem;
        s_shader = OpenGL::CreateShader(
            fs::path("Assets/shaders/water.vert"),
            fs::path("Assets/shaders/water.frag"));
    }
    ++s_refCount;

    // Per-instance quad: 4 vertices × (ndc_x, ndc_y, u, v)
    GL::GenVertexArrays(1, &_vao);
    GL::GenBuffers(1, &_vbo);

    GL::BindVertexArray(_vao);
    GL::BindBuffer(GL_ARRAY_BUFFER, _vbo);
    GL::BufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // location 0 : a_ndc_pos (vec2)
    GL::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                            reinterpret_cast<void*>(0));
    GL::EnableVertexAttribArray(0);

    // location 1 : a_uv (vec2)
    GL::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                            reinterpret_cast<void*>(2 * sizeof(float)));
    GL::EnableVertexAttribArray(1);

    GL::BindVertexArray(0);
    GL::BindBuffer(GL_ARRAY_BUFFER, 0);
}

// ── Logic ─────────────────────────────────────────────────────────────────────

double WaterZone::GetSubmergedDepth(double worldX, double worldBottomY) const
{
    const Math::vec2 c  = GetPosition();
    const double     hw = size.x * 0.5;
    const double     hh = size.y * 0.5;

    if (worldX < c.x - hw || worldX > c.x + hw) return 0.0;

    const double surfaceY = c.y + hh;
    if (worldBottomY >= surfaceY) return 0.0;

    return std::max(0.0, std::min(surfaceY - worldBottomY, size.y));
}

void WaterZone::Update(double dt)
{
    waveTime += dt;
    CS230::GameObject::Update(dt);
}

// ── Render ────────────────────────────────────────────────────────────────────

void WaterZone::Draw(const Math::TransformationMatrix& camera_matrix)
{
    const Math::vec2 c  = GetPosition();
    const double     hw = size.x * 0.5;
    const double     hh = size.y * 0.5;

    // Transform world-space corners → NDC via the VP matrix
    const Math::vec2 bl = camera_matrix * Math::vec2{ c.x - hw, c.y - hh };  // UV (0,0)
    const Math::vec2 br = camera_matrix * Math::vec2{ c.x + hw, c.y - hh };  // UV (1,0)
    const Math::vec2 tr = camera_matrix * Math::vec2{ c.x + hw, c.y + hh };  // UV (1,1)
    const Math::vec2 tl = camera_matrix * Math::vec2{ c.x - hw, c.y + hh };  // UV (0,1)

    // Upload quad (TRIANGLE_FAN order: bl → br → tr → tl)
    const float verts[16] = {
        static_cast<float>(bl.x), static_cast<float>(bl.y),  0.f, 0.f,
        static_cast<float>(br.x), static_cast<float>(br.y),  1.f, 0.f,
        static_cast<float>(tr.x), static_cast<float>(tr.y),  1.f, 1.f,
        static_cast<float>(tl.x), static_cast<float>(tl.y),  0.f, 1.f,
    };

    GL::BindBuffer(GL_ARRAY_BUFFER, _vbo);
    GL::BufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    GL::BindBuffer(GL_ARRAY_BUFFER, 0);

    // Draw
    GL::UseProgram(s_shader.Shader);

    GLint timeLoc = GL::GetUniformLocation(s_shader.Shader, "u_time");
    if (timeLoc >= 0)
        GL::Uniform1f(timeLoc, static_cast<float>(waveTime));

    GL::Enable(GL_BLEND);
    GL::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GL::BindVertexArray(_vao);
    GL::DrawArrays(GL_TRIANGLE_FAN, 0, 4);
    GL::BindVertexArray(0);

    GL::UseProgram(0);
}
