#pragma once
#include <vector>
#include <glad/glad.h>  // or your GL loader

// ─── Parameters (tweak at runtime) ───────────────────────────────────────────

struct OriPostProcessParams
{
    // Bloom
    float bloomThreshold  = 0.8f;
    float bloomIntensity  = 1.0f;
    int   bloomIterations = 4;     // downsample depth

    // Contrast / Brightness
    float contrast   = 1.0f;
    float brightness = 0.0f;

    // Per-channel bezier color correction (4 control points, range 0-1)
    float bezierR[4] = { 0.0f, 0.33f, 0.66f, 1.0f };
    float bezierG[4] = { 0.0f, 0.33f, 0.66f, 1.0f };
    float bezierB[4] = { 0.0f, 0.33f, 0.66f, 1.0f };

    // Desaturation  [0,1]
    float desaturation = 0.0f;

    // Film grain
    float grainOffsetX = 0.0f, grainOffsetY = 0.0f;
    float grainScaleX  = 10.0f, grainScaleY = 10.0f;

    // Directional / motion blur
    float blurStrength = 0.0f;
    float blurDirX = 1.0f, blurDirY = 0.0f;

    // Twirl
    float twirlAngle    = 0.0f;  // radians
    float twirlCenterX  = 0.5f, twirlCenterY = 0.5f;
    float twirlRadiusX  = 0.3f, twirlRadiusY = 0.3f;
};

// ─── FBO helper ──────────────────────────────────────────────────────────────

struct FboTarget
{
    GLuint fbo = 0;
    GLuint tex = 0;
    int    w = 0, h = 0;

    void create(int width, int height)
    {
        w = width; h = height;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0,
                     GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, tex, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void destroy()
    {
        if (fbo) glDeleteFramebuffers(1, &fbo);
        if (tex) glDeleteTextures(1, &tex);
        fbo = tex = 0;
    }

    void bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, w, h);
    }
};

// ─── Post Processing Renderer ─────────────────────────────────────────────────

class OriPostProcessing
{
public:
    // 초기화: shader program ID들을 외부에서 컴파일해서 넘겨줌
    void init(int screenW, int screenH,
              GLuint progBrightPass,
              GLuint progGaussBlur,
              GLuint progUpsample,
              GLuint progComposite,
              GLuint grainTexture)
    {
        _w = screenW; _h = screenH;
        _progBright    = progBrightPass;
        _progBlur      = progGaussBlur;
        _progUpsample  = progUpsample;
        _progComposite = progComposite;
        _grainTex      = grainTexture;

        // fullscreen VAO (버텍스 없이 gl_VertexID 사용)
        glGenVertexArrays(1, &_vao);

        rebuildPyramid();
    }

    void resize(int w, int h)
    {
        _w = w; _h = h;
        rebuildPyramid();
    }

    // sceneTex: 씬이 렌더된 텍스처 ID
    // outputFbo: 0이면 기본 프레임버퍼(화면)에 출력
    void render(GLuint sceneTex, GLuint outputFbo, const OriPostProcessParams& p)
    {
        glBindVertexArray(_vao);

        // ── 1. BrightPass ─────────────────────────────────────────────────
        _pyramid[0].bind();
        useProg(_progBright);
        bindTex(0, sceneTex, "u_scene");
        setFloat("u_threshold", p.bloomThreshold);
        draw();

        // ── 2. Downsample + GaussBlur (H then V) ──────────────────────────
        for (int i = 0; i < p.bloomIterations - 1; i++)
        {
            // ping = source, pong = temp
            FboTarget& src  = _pyramid[i];
            FboTarget& ping = _pyramid[i + 1];
            FboTarget& pong = _pong[i + 1];

            useProg(_progBlur);

            // H pass: src → pong
            pong.bind();
            bindTex(0, src.tex);
            setVec2("u_texel_size", 1.0f / pong.w, 1.0f / pong.h);
            setVec2("u_direction",  1.0f, 0.0f);
            draw();

            // V pass: pong → ping
            ping.bind();
            bindTex(0, pong.tex);
            setVec2("u_texel_size", 1.0f / ping.w, 1.0f / ping.h);
            setVec2("u_direction",  0.0f, 1.0f);
            draw();
        }

        // ── 3. Upsample + accumulate (additive) ───────────────────────────
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        useProg(_progUpsample);

        for (int i = p.bloomIterations - 1; i > 0; i--)
        {
            _pyramid[i - 1].bind();
            bindTex(0, _pyramid[i].tex);
            setVec2("u_texel_size", 1.0f / _pyramid[i].w, 1.0f / _pyramid[i].h);
            setFloat("u_radius", 0.5f);
            draw();
        }
        glDisable(GL_BLEND);

        // ── 4. Composite ──────────────────────────────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, outputFbo);
        glViewport(0, 0, _w, _h);
        useProg(_progComposite);

        bindTex(0, sceneTex,        "u_scene");
        bindTex(1, _pyramid[0].tex, "u_bloom");
        bindTex(2, _grainTex,       "u_grain");

        setFloat("u_bloom_intensity", p.bloomIntensity);
        setFloat("u_contrast",        p.contrast);
        setFloat("u_brightness",      p.brightness);
        setVec4("u_bezier_r",  p.bezierR[0], p.bezierR[1], p.bezierR[2], p.bezierR[3]);
        setVec4("u_bezier_g",  p.bezierG[0], p.bezierG[1], p.bezierG[2], p.bezierG[3]);
        setVec4("u_bezier_b",  p.bezierB[0], p.bezierB[1], p.bezierB[2], p.bezierB[3]);
        setFloat("u_desat",           p.desaturation);
        setVec4("u_grain_offset_scale",
                p.grainOffsetX, p.grainOffsetY, p.grainScaleX, p.grainScaleY);
        setFloat("u_blur_strength",   p.blurStrength);
        setVec2("u_blur_dir",         p.blurDirX, p.blurDirY);
        setFloat("u_twirl_angle",     p.twirlAngle);
        setVec4("u_twirl_center_radius",
                p.twirlCenterX, p.twirlCenterY, p.twirlRadiusX, p.twirlRadiusY);

        draw();

        glBindVertexArray(0);
    }

    void destroy()
    {
        for (auto& t : _pyramid) t.destroy();
        for (auto& t : _pong)    t.destroy();
        glDeleteVertexArrays(1, &_vao);
    }

private:
    int    _w = 0, _h = 0;
    GLuint _vao = 0;
    GLuint _progBright = 0, _progBlur = 0, _progUpsample = 0, _progComposite = 0;
    GLuint _grainTex = 0;

    std::vector<FboTarget> _pyramid;  // downsample chain
    std::vector<FboTarget> _pong;     // blur ping-pong temp

    void rebuildPyramid()
    {
        // bloom 피라미드 최대 6단
        int maxIter = 6;
        _pyramid.resize(maxIter);
        _pong.resize(maxIter);
        int w = _w / 2, h = _h / 2;
        for (int i = 0; i < maxIter; i++)
        {
            _pyramid[i].destroy();
            _pong[i].destroy();
            _pyramid[i].create(std::max(1, w), std::max(1, h));
            _pong[i].create(std::max(1, w), std::max(1, h));
            w /= 2; h /= 2;
        }
    }

    void draw()
    {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    GLuint _currentProg = 0;

    void useProg(GLuint prog)
    {
        _currentProg = prog;
        glUseProgram(prog);
    }

    void bindTex(int slot, GLuint tex, const char* samplerName = nullptr)
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, tex);
        if (samplerName)
        {
            GLint loc = glGetUniformLocation(_currentProg, samplerName);
            if (loc >= 0) glUniform1i(loc, slot);
        }
    }

    void setFloat(const char* n, float v)
    {
        GLint loc = glGetUniformLocation(_currentProg, n);
        if (loc >= 0) glUniform1f(loc, v);
    }

    void setVec2(const char* n, float x, float y)
    {
        GLint loc = glGetUniformLocation(_currentProg, n);
        if (loc >= 0) glUniform2f(loc, x, y);
    }

    void setVec4(const char* n, float x, float y, float z, float w)
    {
        GLint loc = glGetUniformLocation(_currentProg, n);
        if (loc >= 0) glUniform4f(loc, x, y, z, w);
    }
};
