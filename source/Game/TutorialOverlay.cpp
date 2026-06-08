#include "TutorialOverlay.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "CS200/RGBA.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Window.hpp"

#include <algorithm>
#include <cmath>

// ── Update ────────────────────────────────────────────────────────────────────

void TutorialOverlay::Update(double dt)
{
    switch (state)
    {
    case State::Idle:
        if (!queue.empty())
        {
            current = queue.front().text;
            holdDur = queue.front().duration;
            curPos  = queue.front().normPos;
            queue.pop_front();
            alpha = 0.0f;
            timer = 0.0;
            state = State::FadeIn;
        }
        break;

    case State::FadeIn:
        timer += dt;
        alpha = static_cast<float>(std::min(timer / FADE_IN, 1.0));
        if (timer >= FADE_IN) { alpha = 1.0f; timer = 0.0; state = State::Hold; }
        break;

    case State::Hold:
        timer += dt;
        if (timer >= holdDur) { timer = 0.0; state = State::FadeOut; }
        break;

    case State::FadeOut:
        timer += dt;
        alpha = static_cast<float>(std::max(1.0 - timer / FADE_OUT, 0.0));
        if (timer >= FADE_OUT) { alpha = 0.0f; state = State::Idle; }
        break;
    }
}

bool TutorialOverlay::IsIdle() const
{
    return state == State::Idle && queue.empty();
}

void TutorialOverlay::Show(const std::string& text, double duration, Math::vec2 normPos)
{
    queue.push_back({ text, duration, normPos });
}

// ── Draw ──────────────────────────────────────────────────────────────────────
// Must be called inside a renderer.BeginScene(screen_matrix) / EndScene block
// where screen_matrix = CS200::build_ndc_matrix(winSize).

void TutorialOverlay::Draw()
{
    if (state == State::Idle || alpha <= 0.001f)
        return;

    [[maybe_unused]] auto& renderer = Engine::GetRenderer2D();
    const Math::ivec2 win = Engine::GetWindow().GetSize();
    const double      W   = static_cast<double>(win.x);
    const double      H   = static_cast<double>(win.y);

    CS230::Font& font    = Engine::GetFont(0);
    auto         textTex = font.PrintToTexture(current, CS200::WHITE);
    if (!textTex) return;

    const Math::ivec2 texSize = textTex->GetSize();
    const double      textW   = static_cast<double>(texSize.x);
    const double      textH   = static_cast<double>(texSize.y);
    const double      boxCX   = W * curPos.x;
    const double      boxCY   = H * curPos.y;

    const auto            ua        = static_cast<uint32_t>(alpha * 255.0f);
    const CS200::RGBA     textColor = (0xFFFFFFu << 8) | ua;

    // ── Text only, no background box ─────────────────────────────────────────
    const double textX = boxCX - textW * 0.5;
    const double textY = boxCY - textH * 0.5;
    const auto   textMat = Math::TranslationMatrix(Math::vec2{ textX, textY })
                         * Math::ScaleMatrix(Math::vec2{ 1.0, 1.0 });
    textTex->Draw(textMat, textColor);
}
