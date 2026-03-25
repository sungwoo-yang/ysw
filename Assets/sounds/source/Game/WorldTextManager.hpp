#pragma once

#include "CS200/RGBA.hpp"
#include "Engine/Component.hpp"
#include "Engine/Vec2.hpp"
#include <string>
#include <vector>

namespace CS230
{
    class Camera;
    class GameObject;
}

namespace CS200
{
    class IRenderer2D;
}

class WorldTextManager : public CS230::Component
{
public:
    WorldTextManager();

    void Update([[maybe_unused]] double dt) override;
    void Draw();
    void SetCamera(CS230::Camera* camera);

    // Queues a text message to be drawn above or below a specific GameObject
    void ShowTextAbove(CS230::GameObject* obj, const std::string& text, double scale, CS200::RGBA color);
    void ShowTextBelow(CS230::GameObject* obj, const std::string& text, double scale, CS200::RGBA color);

private:
    // Represents a single frame's text rendering request
    struct TextJob
    {
        std::string text;
        Math::vec2  worldPos;
        bool        alignAbove; // True for above the object, false for below
        double      scale;
        CS200::RGBA color;
    };

    // Converts world coordinates to screen/window coordinates for 2D UI rendering
    Math::vec2 WorldToScreen(Math::vec2 worldPos);

    std::vector<TextJob> textJobs;  // Queue of text to draw this frame
    CS230::Camera*       camera;
};