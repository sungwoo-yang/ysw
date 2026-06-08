#pragma once
#include "OpenGL/GL.hpp"
#include "OpenGL/Shader.hpp"
#include "Engine/Vec2.hpp"
#include <array>

// Multi-pass post-processing pipeline:
//   Bloom (BrightPass → Gaussian downsample → tent upsample → composite)
//   God Rays (occluder bright-pass → radial blur → additive blit)
class OriPostProcessor {
public:
    void Initialize(Math::ivec2 windowSize);
    void Shutdown();

    void BeginSceneRender();   // bind scene FBO, clear
    void EndRenderAndDraw();   // run full pipeline → screen (FBO 0)

    // ---- Bloom ----
    float bloomThreshold  = 0.62f;
    float bloomIntensity  = 0.65f;
    int   bloomIterations = 4;
    float contrast        = 1.0f;
    float brightness      = 0.0f;
    float desaturation    = 0.0f;

    // ---- Health vignette & death blackout ----
    void SetPlayerHp(float hp, float maxHp) { _playerHp = hp; _maxHp = maxHp; }
    void SetBlackout(float t)               { _blackout = t; }
    void SetCameraPos(float x, float y)     { _camX = x; _camY = y; }
    void Tick(double dt)                    { _vigTime += static_cast<float>(dt); }

    // ---- God Rays ----
    bool  godRaysEnabled = true;
    float lightPosX      = 0.5f;
    float lightPosY      = 0.88f;
    float raysExposure   = 0.28f;
    float raysDecay      = 0.97f;
    float raysDensity    = 0.7f;
    float raysWeight     = 0.005f;
    int   raysSamples    = 64;

private:
    static constexpr int MAX_BLOOM = 6;

    struct Fbo {
        GLuint fbo = 0, tex = 0;
        int    w = 0, h = 0;

        void create(int width, int height);
        void destroy();
        void bind() const;
    };

    int _w = 0, _h = 0;

    Fbo _scene;                              // full-res scene
    std::array<Fbo, MAX_BLOOM> _bloomPyr;    // downsample chain
    std::array<Fbo, MAX_BLOOM> _bloomPong;   // blur ping-pong temps
    Fbo _occFbo;                             // god rays occluder  (1/2)
    Fbo _raysFbo;                            // god rays result    (1/2)

    OpenGL::CompiledShader _sBright;
    OpenGL::CompiledShader _sBlur;
    OpenGL::CompiledShader _sUpsample;
    OpenGL::CompiledShader _sComposite;
    OpenGL::CompiledShader _sGodRays;
    OpenGL::CompiledShader _sBlit;     // simple passthrough for additive blit
    OpenGL::CompiledShader _sVignette; // health vignette overlay

    float _playerHp = 5.0f;
    float _maxHp    = 5.0f;
    float _vigTime  = 0.0f;
    float _blackout = 0.0f;
    float _camX     = 0.0f;
    float _camY     = 0.0f;

    GLuint _fsVao    = 0;
    GLuint _grainTex = 0;

    void allocFbos(int w, int h);
    void freeFbos();
    void genGrainTex();

    void use(const OpenGL::CompiledShader& s) const;
    void setf (const OpenGL::CompiledShader& s, const char* n, float v) const;
    void setv2(const OpenGL::CompiledShader& s, const char* n, float x, float y) const;
    void seti (const OpenGL::CompiledShader& s, const char* n, int v) const;
    void bindTex(const OpenGL::CompiledShader& s, int slot, GLuint tex, const char* name) const;
    void drawFs() const;
};
