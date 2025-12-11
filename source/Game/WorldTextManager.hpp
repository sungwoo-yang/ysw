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
    void ShowTextAbove(CS230::GameObject* obj, const std::string& text, double scale, CS200::RGBA color);
    void ShowTextBelow(CS230::GameObject* obj, const std::string& text, double scale, CS200::RGBA color);

private:
    struct TextJob
    {
        std::string text;
        Math::vec2  worldPos;
        bool        alignAbove;
        double      scale;
        CS200::RGBA color;
    };

    Math::vec2 WorldToScreen(Math::vec2 worldPos);

    std::vector<TextJob> textJobs;
    CS230::Camera*       camera;
};