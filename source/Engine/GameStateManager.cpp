/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "GameStateManager.hpp"
#include "Engine.hpp"
#include "GameObjectManager.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "CS200/RGBA.hpp"
#include "Window.hpp"

namespace CS230
{
    void GameStateManager::PopState()
    {
        if (is_updating)
        {
            pending_actions.push_back([this]() { PopState(); });
            return;
        }

        using namespace std::literals;
        auto* const state = mGameStateStack.back().get();
        mToClear.push_back(std::move(mGameStateStack.back()));
        mGameStateStack.erase(mGameStateStack.end() - 1);
        Engine::GetLogger().LogEvent("Exiting state "s + state->GetName());
        state->Unload();
    }

    void GameStateManager::Update(double dt)
    {
        for (const auto& action : pending_actions)
        {
            action();
        }
        pending_actions.clear();

        mToClear.clear();

        if (mFadeState == FadeState::FadeOut)
        {
            mFadeAlpha += static_cast<float>(mFadeSpeed * dt);
            if (mFadeAlpha >= 1.0f)
            {
                mFadeAlpha = 1.0f;
                if (mFadeAction)
                {
                    mFadeAction();
                    mFadeAction = nullptr;
                }
                mFadeState = FadeState::FadeIn;
            }
        }
        else if (mFadeState == FadeState::FadeIn)
        {
            if (!mHoldFadeIn) 
            {
                mFadeAlpha -= static_cast<float>(mFadeSpeed * dt);
                if (mFadeAlpha <= 0.0f)
                {
                    mFadeAlpha = 0.0f;
                    mFadeState = FadeState::None;
                }
            }
        }

        if (mGameStateStack.empty())
        {
            return;
        }
        is_updating = true;

        mGameStateStack.back()->Update(dt);

        GameObjectManager* gom = mGameStateStack.back()->GetGSComponent<GameObjectManager>();
        if (gom != nullptr)
        {
            gom->CollisionTest();
        }

        is_updating = false;
    }
    void GameStateManager::Draw()
    {
        for (auto& game_state : mGameStateStack)
        {
            game_state->Draw();
        }

        if (mFadeAlpha > 0.0f)
        {
            auto& renderer = Engine::GetRenderer2D();
            Math::ivec2 display_size = Engine::GetWindow().GetSize();

            renderer.BeginScene(CS200::build_ndc_matrix(display_size));

            CS200::RGBA fadeColor = CS200::pack_color({0.0f, 0.0f, 0.0f, mFadeAlpha});
            
            double width = static_cast<double>(display_size.x);
            double height = static_cast<double>(display_size.y);
            
            Math::TransformationMatrix transform = Math::TranslationMatrix(Math::vec2{width / 2.0, height / 2.0}) * Math::ScaleMatrix(Math::vec2{width, height});

            renderer.DrawRectangle(transform, fadeColor, CS200::CLEAR, 0.0);

            renderer.EndScene();
        }
    }

    void GameStateManager::DrawImGui()
    {
        mGameStateStack.back()->DrawImGui();
    }

    void GameStateManager::Clear()
    {
        if (is_updating)
        {
            pending_actions.push_back([this]() { Clear(); });
            return;
        }

        while (!mGameStateStack.empty())
            PopState();
        mToClear.clear();
    }
}