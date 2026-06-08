#include "OriPostProcessor.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Path.hpp"
#include "Engine/Window.hpp"
#include "OpenGL/GL.hpp"
#include "OpenGL/Shader.hpp"

#include <algorithm>
#include <filesystem>
#include <random>
#include <vector>


// ──────────────────────────────────────────────────────────────────────────────
// Fbo helpers
// ──────────────────────────────────────────────────────────────────────────────

void OriPostProcessor::Fbo::create(int width, int height)
{
    w = width; h = height;

    GL::GenTextures(1, &tex);
    GL::BindTexture(GL_TEXTURE_2D, tex);
    GL::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
    GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GL::BindTexture(GL_TEXTURE_2D, 0);

    GL::GenFramebuffers(1, &fbo);
    GL::BindFramebuffer(GL_FRAMEBUFFER, fbo);
    GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OriPostProcessor::Fbo::destroy()
{
    if (fbo) GL::DeleteFramebuffers(1, &fbo);
    if (tex) GL::DeleteTextures(1, &tex);
    fbo = tex = w = h = 0;
}

void OriPostProcessor::Fbo::bind() const
{
    GL::BindFramebuffer(GL_FRAMEBUFFER, fbo);
    GL::Viewport(0, 0, w, h);
}

// ──────────────────────────────────────────────────────────────────────────────
// FBO allocation / teardown
// ──────────────────────────────────────────────────────────────────────────────

void OriPostProcessor::allocFbos(int w, int h)
{
    _scene.create(w, h);

    // Bloom pyramid: each level is half the previous
    int pw = w / 2, ph = h / 2;
    for (int i = 0; i < MAX_BLOOM; ++i)
    {
        _bloomPyr [i].create(std::max(1, pw), std::max(1, ph));
        _bloomPong[i].create(std::max(1, pw), std::max(1, ph));
        pw /= 2; ph /= 2;
    }

    _occFbo .create(std::max(1, w / 2), std::max(1, h / 2));
    _raysFbo.create(std::max(1, w / 2), std::max(1, h / 2));
}

void OriPostProcessor::freeFbos()
{
    _scene.destroy();
    for (auto& f : _bloomPyr)  f.destroy();
    for (auto& f : _bloomPong) f.destroy();
    _occFbo .destroy();
    _raysFbo.destroy();
}

// ──────────────────────────────────────────────────────────────────────────────
// Grain texture (256×256 random noise, single channel)
// ──────────────────────────────────────────────────────────────────────────────

void OriPostProcessor::genGrainTex()
{
    std::mt19937 rng(0xDEAD1234u);
    std::uniform_int_distribution<int> dist(0, 255);

    std::vector<uint8_t> data(256 * 256);
    for (auto& b : data) b = static_cast<uint8_t>(dist(rng));

    GL::GenTextures(1, &_grainTex);
    GL::BindTexture(GL_TEXTURE_2D, _grainTex);
    GL::TexImage2D(GL_TEXTURE_2D, 0, GL_R8, 256, 256, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
    GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GL::BindTexture(GL_TEXTURE_2D, 0);
}

// ──────────────────────────────────────────────────────────────────────────────
// Uniform helpers
// ──────────────────────────────────────────────────────────────────────────────

void OriPostProcessor::use(const OpenGL::CompiledShader& s) const
{
    GL::UseProgram(s.Shader);
}

void OriPostProcessor::setf(const OpenGL::CompiledShader& s, const char* n, float v) const
{
    GLint loc = GL::GetUniformLocation(s.Shader, n);
    if (loc >= 0) GL::Uniform1f(loc, v);
}

void OriPostProcessor::setv2(const OpenGL::CompiledShader& s, const char* n, float x, float y) const
{
    GLint loc = GL::GetUniformLocation(s.Shader, n);
    if (loc >= 0) GL::Uniform2f(loc, x, y);
}

void OriPostProcessor::seti(const OpenGL::CompiledShader& s, const char* n, int v) const
{
    GLint loc = GL::GetUniformLocation(s.Shader, n);
    if (loc >= 0) GL::Uniform1i(loc, v);
}

void OriPostProcessor::bindTex(const OpenGL::CompiledShader& s, int slot,
                                GLuint tex, const char* name) const
{
    GL::ActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(slot));
    GL::BindTexture(GL_TEXTURE_2D, tex);
    if (name)
    {
        GLint loc = GL::GetUniformLocation(s.Shader, name);
        if (loc >= 0) GL::Uniform1i(loc, slot);
    }
}

void OriPostProcessor::drawFs() const
{
    GL::BindVertexArray(_fsVao);
    GL::DrawArrays(GL_TRIANGLES, 0, 3);
}

// ──────────────────────────────────────────────────────────────────────────────
// Initialize / Shutdown
// ──────────────────────────────────────────────────────────────────────────────

void OriPostProcessor::Initialize(Math::ivec2 windowSize)
{
    _w = windowSize.x;
    _h = windowSize.y;

    namespace fs = std::filesystem;
    const fs::path shaderDir = "Assets/shaders";

    _sBright    = OpenGL::CreateShader(shaderDir / "fullscreen.vert",  shaderDir / "bright_pass.frag");
    _sBlur      = OpenGL::CreateShader(shaderDir / "fullscreen.vert",  shaderDir / "gaussian_blur.frag");
    _sUpsample  = OpenGL::CreateShader(shaderDir / "fullscreen.vert",  shaderDir / "upsample.frag");
    _sComposite = OpenGL::CreateShader(shaderDir / "fullscreen.vert",  shaderDir / "composite.frag");
    _sGodRays   = OpenGL::CreateShader(shaderDir / "fullscreen.vert",  shaderDir / "god_rays.frag");

    // Inline passthrough shader for additive god rays blit
    static constexpr std::string_view BLIT_VERT = R"(
#version 330 core
out vec2 v_uv;
void main() {
    vec2 pos = vec2((gl_VertexID==1)?3.0:-1.0, (gl_VertexID==2)?3.0:-1.0);
    v_uv = pos*0.5+0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
})";
    static constexpr std::string_view BLIT_FRAG = R"(
#version 330 core
in vec2 v_uv;
out vec4 fragColor;
uniform sampler2D u_tex;
void main() { fragColor = vec4(texture(u_tex, v_uv).rgb, 1.0); })";

    _sBlit     = OpenGL::CreateShader(BLIT_VERT, BLIT_FRAG);
    _sVignette = OpenGL::CreateShader(shaderDir / "fullscreen.vert",
                                      shaderDir / "health_vignette.frag");

    // Empty VAO for VAO-less fullscreen draws (uses gl_VertexID)
    GL::GenVertexArrays(1, &_fsVao);

    genGrainTex();
    allocFbos(_w, _h);
}

void OriPostProcessor::Shutdown()
{
    freeFbos();

    OpenGL::DestroyShader(_sBright);
    OpenGL::DestroyShader(_sBlur);
    OpenGL::DestroyShader(_sUpsample);
    OpenGL::DestroyShader(_sComposite);
    OpenGL::DestroyShader(_sGodRays);
    OpenGL::DestroyShader(_sBlit);
    OpenGL::DestroyShader(_sVignette);

    if (_fsVao)    GL::DeleteVertexArrays(1, &_fsVao);
    if (_grainTex) GL::DeleteTextures(1, &_grainTex);
    _fsVao = _grainTex = 0;
}

// ──────────────────────────────────────────────────────────────────────────────
// Per-frame API
// ──────────────────────────────────────────────────────────────────────────────

void OriPostProcessor::BeginSceneRender()
{
    _scene.bind();
    Engine::GetWindow().Clear(0x000000FF);
}

void OriPostProcessor::EndRenderAndDraw()
{
    const int iters = std::clamp(bloomIterations, 1, MAX_BLOOM);

    // ── Step 1: God Rays occluder (bright-pass of scene at 1/2 res) ──────────
    if (godRaysEnabled)
    {
        _occFbo.bind();
        use(_sBright);
        bindTex(_sBright, 0, _scene.tex, "u_scene");
        setf(_sBright, "u_threshold", bloomThreshold * 1.8f); // tighter threshold for rays
        drawFs();

        // God Rays pass
        _raysFbo.bind();
        use(_sGodRays);
        bindTex(_sGodRays, 0, _occFbo.tex, "u_occlusion");
        setv2(_sGodRays, "u_light_pos", lightPosX, lightPosY);
        seti (_sGodRays, "u_num_samples", raysSamples);
        setf (_sGodRays, "u_density",  raysDensity);
        setf (_sGodRays, "u_weight",   raysWeight);
        setf (_sGodRays, "u_decay",    raysDecay);
        setf (_sGodRays, "u_exposure", raysExposure);
        drawFs();
    }

    // ── Step 2: Bloom — BrightPass scene → pyramid[0] ────────────────────────
    _bloomPyr[0].bind();
    use(_sBright);
    bindTex(_sBright, 0, _scene.tex, "u_scene");
    setf(_sBright, "u_threshold", bloomThreshold);
    drawFs();

    // ── Step 3: Bloom — Downsample + Gaussian blur ───────────────────────────
    for (int i = 0; i < iters - 1; ++i)
    {
        const Fbo& src  = _bloomPyr [i];
        const Fbo& ping = _bloomPyr [i + 1];
        const Fbo& pong = _bloomPong[i + 1];

        use(_sBlur);

        // H pass: src → pong (same resolution as ping)
        const_cast<Fbo&>(pong).bind();
        bindTex(_sBlur, 0, src.tex, "u_tex");
        setv2(_sBlur, "u_texel_size", 1.0f / static_cast<float>(pong.w),
                                       1.0f / static_cast<float>(pong.h));
        setv2(_sBlur, "u_direction", 1.0f, 0.0f);
        drawFs();

        // V pass: pong → ping
        const_cast<Fbo&>(ping).bind();
        bindTex(_sBlur, 0, pong.tex, "u_tex");
        setv2(_sBlur, "u_texel_size", 1.0f / static_cast<float>(ping.w),
                                       1.0f / static_cast<float>(ping.h));
        setv2(_sBlur, "u_direction", 0.0f, 1.0f);
        drawFs();
    }

    // ── Step 4: Bloom — Upsample + accumulate (additive) ─────────────────────
    GL::Enable(GL_BLEND);
    GL::BlendFunc(GL_ONE, GL_ONE);
    use(_sUpsample);

    for (int i = iters - 1; i > 0; --i)
    {
        const_cast<Fbo&>(_bloomPyr[i - 1]).bind();
        bindTex(_sUpsample, 0, _bloomPyr[i].tex, "u_tex");
        setv2(_sUpsample, "u_texel_size",
              1.0f / static_cast<float>(_bloomPyr[i].w),
              1.0f / static_cast<float>(_bloomPyr[i].h));
        setf(_sUpsample, "u_radius", 0.5f);
        drawFs();
    }

    GL::Disable(GL_BLEND);

    // ── Step 5: Composite → screen ────────────────────────────────────────────
    GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
    GL::Viewport(0, 0, _w, _h);
    use(_sComposite);

    bindTex(_sComposite, 0, _scene.tex,       "u_scene");
    bindTex(_sComposite, 1, _bloomPyr[0].tex, "u_bloom");
    bindTex(_sComposite, 2, _grainTex,        "u_grain");

    setf (_sComposite, "u_bloom_intensity", bloomIntensity);
    setf (_sComposite, "u_contrast",        contrast);
    setf (_sComposite, "u_brightness",      brightness);

    // Identity bezier curves (linear passthrough)
    GLint locR = GL::GetUniformLocation(_sComposite.Shader, "u_bezier_r");
    GLint locG = GL::GetUniformLocation(_sComposite.Shader, "u_bezier_g");
    GLint locB = GL::GetUniformLocation(_sComposite.Shader, "u_bezier_b");
    const float bezier[4] = { 0.0f, 0.33f, 0.66f, 1.0f };
    if (locR >= 0) GL::Uniform4fv(locR, 1, bezier);
    if (locG >= 0) GL::Uniform4fv(locG, 1, bezier);
    if (locB >= 0) GL::Uniform4fv(locB, 1, bezier);

    setf(_sComposite, "u_desat",          desaturation);
    setf(_sComposite, "u_blur_strength",  0.0f);
    setv2(_sComposite,"u_blur_dir",       1.0f, 0.0f);
    setf(_sComposite, "u_twirl_angle",    0.0f);

    GLint locTwirl = GL::GetUniformLocation(_sComposite.Shader, "u_twirl_center_radius");
    if (locTwirl >= 0) GL::Uniform4f(locTwirl, 0.5f, 0.5f, 0.3f, 0.3f);

    // World-space grain: offset by camera position so grain moves with the world.
    // Dividing by resolution * scale anchors the grain pattern to world coordinates.
    const float grainOffX = _camX / (static_cast<float>(_w) / 10.0f);
    const float grainOffY = _camY / (static_cast<float>(_h) / 10.0f);
    GLint locGrain = GL::GetUniformLocation(_sComposite.Shader, "u_grain_offset_scale");
    if (locGrain >= 0) GL::Uniform4f(locGrain, grainOffX, grainOffY, 10.0f, 10.0f);

    drawFs();

    // ── Step 6: God Rays — additive blit onto screen ─────────────────────────
    if (godRaysEnabled)
    {
        GL::Enable(GL_BLEND);
        GL::BlendFunc(GL_ONE, GL_ONE);

        use(_sBlit);
        bindTex(_sBlit, 0, _raysFbo.tex, "u_tex");
        drawFs();

        GL::Disable(GL_BLEND);
        GL::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // ── Step 7: Health vignette + death blackout overlay ─────────────────────
    {
        GL::Enable(GL_BLEND);
        GL::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        use(_sVignette);
        setf(_sVignette, "u_hp",       _playerHp);
        setf(_sVignette, "u_max_hp",   _maxHp);
        setf(_sVignette, "u_time",     _vigTime);
        setf(_sVignette, "u_blackout", _blackout);
        drawFs();

        GL::Disable(GL_BLEND);
    }

    GL::BindVertexArray(0);
    GL::UseProgram(0);
    GL::ActiveTexture(GL_TEXTURE0);
    GL::BindTexture(GL_TEXTURE_2D, 0);
}
