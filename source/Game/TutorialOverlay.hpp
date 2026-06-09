#pragma once
#include "Engine/Component.hpp"
#include "Engine/Vec2.hpp"
#include <deque>
#include <string>

// Screen-space tutorial text overlay.
// normPos: normalized screen position (x 0=left..1=right, y 0=bottom..1=top)
// Default {0.5, 0.16} = center-bottom.
class TutorialOverlay : public CS230::Component
{
public:
    void Update(double dt) override;
    void Draw();

    // Queue a message at a normalized screen position.
    void Show(const std::string& text, double duration = 3.0,
              Math::vec2 normPos = { 0.5, 0.16 });

    // Instantly dismiss current text and clear queue.
    void Clear();

    bool IsIdle() const;

private:
    enum class State { Idle, FadeIn, Hold, FadeOut };

    struct Message
    {
        std::string text;
        double      duration;
        Math::vec2  normPos = { 0.5, 0.16 };
    };

    std::deque<Message> queue;
    State       state    = State::Idle;
    double      timer    = 0.0;
    double      holdDur  = 0.0;
    float       alpha    = 0.0f;
    std::string current;
    Math::vec2  curPos   = { 0.5, 0.16 };

    static constexpr double FADE_IN  = 0.35;
    static constexpr double FADE_OUT = 0.35;
};
