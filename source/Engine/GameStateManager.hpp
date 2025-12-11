/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine.hpp"
#include "GameState.hpp"
#include "Logger.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace CS230
{
    class GameStateManager
    {
    public:
        template <typename STATE>
        void PushState();
        void PopState();
        void Update(double dt);
        void Draw();
        void DrawImGui();

        [[nodiscard]] bool HasGameEnded() const
        {
            return mGameStateStack.empty();
        }

        void Clear();

        template <typename T>
        T* GetGSComponent()
        {
            if (mGameStateStack.empty())
            {
                return nullptr;
            }

            return mGameStateStack.back()->GetGSComponent<T>();
        }

        gsl ::czstring GetCurrentStateName() const
        {
            return mGameStateStack.empty() ? "None" : mGameStateStack.back()->GetName();
        }

    private:
        std::vector<std::unique_ptr<GameState>> mGameStateStack;
        std::vector<std::unique_ptr<GameState>> mToClear;

        bool                               is_updating = false;
        std::vector<std::function<void()>> pending_actions;
    };

    template <typename STATE>
    void GameStateManager::PushState()
    {
        if (is_updating)
        {
            pending_actions.push_back([this]() { PushState<STATE>(); });
            return;
        }

        using namespace std::literals;
        mGameStateStack.push_back(std::make_unique<STATE>());
        const auto& state = mGameStateStack.back();
        Engine::GetLogger().LogEvent("Entering state "s + state->GetName());
        state->Load();
    }
}
