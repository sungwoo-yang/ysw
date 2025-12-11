#include "WorldTextManager.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Texture.hpp"
#include "Engine/Window.hpp"

WorldTextManager::WorldTextManager() : camera(nullptr)
{
}

void WorldTextManager::Update([[maybe_unused]] double dt)
{
    textJobs.clear();
}

void WorldTextManager::SetCamera(CS230::Camera* cam)
{
    this->camera = cam;
}

Math::vec2 WorldTextManager::WorldToScreen(Math::vec2 worldPos)
{
    if (camera == nullptr)
    {
        return { -1000.0, -1000.0 };
    }

    Math::TransformationMatrix viewProj = CS200::build_ndc_matrix(Engine::GetWindow().GetSize()) * camera->GetMatrix();
    Math::vec2                 ndcPos   = viewProj * worldPos;
    Math::ivec2                winSize  = Engine::GetWindow().GetSize();

    double screenX = (ndcPos.x + 1.0) * 0.5 * static_cast<double>(winSize.x);
    double screenY = (ndcPos.y + 1.0) * 0.5 * static_cast<double>(winSize.y);

    return { screenX, screenY };
}

void WorldTextManager::ShowTextAbove(CS230::GameObject* obj, const std::string& text, double scale, CS200::RGBA color)
{
    if (obj == nullptr)
        return;

    Math::vec2 pos      = obj->GetPosition();
    auto       collider = obj->GetGOComponent<CS230::RectCollision>();
    if (collider)
    {
        pos.x = obj->GetPosition().x;
        pos.y = collider->WorldBoundary().Top() + 100.0;
    }

    textJobs.push_back({ text, pos, true, scale, color });
}

void WorldTextManager::ShowTextBelow(CS230::GameObject* obj, const std::string& text, double scale, CS200::RGBA color)
{
    if (obj == nullptr)
        return;

    Math::vec2 pos      = obj->GetPosition();
    auto       collider = obj->GetGOComponent<CS230::RectCollision>();
    if (collider)
    {
        pos.x = obj->GetPosition().x;
        pos.y = collider->WorldBoundary().Bottom() - 15.0;
    }

    textJobs.push_back({ text, pos, false, scale, color });
}

void WorldTextManager::Draw()
{
    if (textJobs.empty())
    {
        return;
    }

    CS230::Font& font = Engine::GetFont(0);

    for (const auto& job : textJobs)
    {
        std::shared_ptr<CS230::Texture> textTexture = font.PrintToTexture(job.text, job.color);

        if (textTexture == nullptr)
        {
            continue;
        }

        Math::ivec2 textureSize = textTexture->GetSize();
        Math::vec2  screenPos   = WorldToScreen(job.worldPos);

        const double scale = job.scale;

        double scaledWidth  = static_cast<double>(textureSize.x) * scale;
        double scaledHeight = static_cast<double>(textureSize.y) * scale;

        Math::vec2 drawPos_BottomLeft;
        drawPos_BottomLeft.x = screenPos.x - scaledWidth * 0.5;

        if (job.alignAbove)
        {
            drawPos_BottomLeft.y = screenPos.y;
        }
        else
        {
            drawPos_BottomLeft.y = screenPos.y - scaledHeight;
        }

        Math::TransformationMatrix transform = Math::TranslationMatrix(drawPos_BottomLeft) * Math::ScaleMatrix(Math::vec2{ scale, scale });

        textTexture->Draw(transform, CS200::WHITE);
    }
}