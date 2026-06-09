#include "BullBoss.hpp"
#include "BreakableWall.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Matrix.hpp"

BullBoss::BullBoss(Math::vec2 pos, BreakableWall* wall, Player* p)
    : CS230::GameObject(pos), entranceWall(wall), player(p)
{
}

void BullBoss::TriggerEntrance()
{
    if (state == State::Idle)
        state = State::Charging;
}

void BullBoss::Update(double dt)
{
    stateTimer += dt;

    switch (state)
    {
    case State::Charging:
    {
        const Math::vec2 pos      = GetPosition();
        const double     wallEdge = entranceWall->GetWorldBounds().Right();
        const double     stopX    = wallEdge + WALL_STOP_DIST + WIDTH * 0.5;

        if (pos.x > stopX)
        {
            SetPosition({ pos.x - CHARGE_SPEED * dt, pos.y });
        }
        else
        {
            SetPosition({ stopX, pos.y });
            entranceWall->Break();
            entranceWall = nullptr;
            stateTimer   = 0.0;
            state        = State::Breaking;
        }
        break;
    }

    case State::Breaking:
        if (stateTimer >= BREAK_WAIT)
        {
            stateTimer = 0.0;
            state      = State::Intro;
        }
        break;

    case State::Intro:
        if (stateTimer >= INTRO_DURATION)
        {
            stateTimer = 0.0;
            state      = State::Chasing;
        }
        break;

    case State::Chasing:
        // Waypoint chase — driven by Mode3
        break;

    default:
        break;
    }

    CS230::GameObject::Update(dt);
}

void BullBoss::Draw(const Math::TransformationMatrix& /*camera_matrix*/)
{
    if (state == State::Idle || state == State::Done) return;

    auto& r = Engine::GetRenderer2D();

    // Flash white during Intro to signal the boss has arrived
    const uint32_t fill = (state == State::Intro &&
                           static_cast<int>(stateTimer * 8.0) % 2 == 0)
                          ? 0xFF4444FFu
                          : 0xAA2222FFu;

    const auto mat = Math::TranslationMatrix(GetPosition())
                   * Math::ScaleMatrix(Math::vec2{ WIDTH, HEIGHT });
    r.DrawRectangle(mat, fill, 0xFF0000FFu, 2.0);
}
