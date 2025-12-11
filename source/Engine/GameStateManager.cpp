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
