#include "BreakableWall.hpp"
#include "Player.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Matrix.hpp"

#include <algorithm>
#include <cmath>

// ============================================================
//  Helpers
// ============================================================
namespace
{
    constexpr double Gravity = 900.0;

    // ---- Triangle scanline fill ----
    void FillTriangle(CS200::IRenderer2D& r,
                      Math::vec2 a, Math::vec2 b, Math::vec2 c,
                      uint32_t color)
    {
        // Sort vertices top→bottom (Y increases upward in this renderer)
        // Use DrawLine horizontal strips — step 2px for performance
        if (a.y > b.y) std::swap(a, b);
        if (a.y > c.y) std::swap(a, c);
        if (b.y > c.y) std::swap(b, c);

        const double step = 2.0;
        const double totalH = c.y - a.y;
        if (totalH < 0.5) return;

        for (double y = a.y; y <= c.y; y += step)
        {
            // Edge a→c (long edge, always present)
            const double tAC = (c.y > a.y) ? (y - a.y) / (c.y - a.y) : 1.0;
            const double xAC = a.x + tAC * (c.x - a.x);

            // Edge a→b or b→c
            double xOther;
            if (y <= b.y)
            {
                const double tAB = (b.y > a.y) ? (y - a.y) / (b.y - a.y) : 1.0;
                xOther = a.x + tAB * (b.x - a.x);
            }
            else
            {
                const double tBC = (c.y > b.y) ? (y - b.y) / (c.y - b.y) : 1.0;
                xOther = b.x + tBC * (c.x - b.x);
            }

            r.DrawLine({ std::min(xAC, xOther), y },
                       { std::max(xAC, xOther), y },
                       color, step);
        }
    }

    // ---- Crack pattern (normalized -0.5..0.5 space) ----
    //   All coords: x * hw*2, y * hh*2 = world offset from wall centre
    //
    //   One crack "centre" + 6 edge points → 6 triangular shards

    constexpr Math::vec2 CRACK_CENTER = { 0.04, -0.06 };

    constexpr Math::vec2 EDGE_PTS[6] = {
        { -0.40,  0.50 },  // 0 – top-left
        {  0.18,  0.50 },  // 1 – top-right
        {  0.50,  0.35 },  // 2 – right-top
        {  0.50, -0.50 },  // 3 – bottom-right
        { -0.08, -0.50 },  // 4 – bottom-center
        { -0.50, -0.20 },  // 5 – left-bottom
    };

    // Precomputed centroid for each shard (CC + EP[i] + EP[i+1]) / 3
    // and their launch parameters
    struct ShardDef {
        int         ep0, ep1;
        Math::vec2  centroid; // normalized
        double      vx, vy, angVel;
    };

    constexpr ShardDef Shards[6] = {
        // shard index, centroid (norm), vx, vy, angVel
        { 0, 1, { -0.06,  0.31 }, -200,  330, -2.5 },  // top-left
        { 1, 2, {  0.24,  0.26 },  200,  310,  3.0 },  // top-right
        { 2, 3, {  0.35, -0.07 },  360,  100, -2.8 },  // right
        { 3, 4, {  0.15, -0.35 },  230,  -90,  2.5 },  // bottom-right
        { 4, 5, { -0.18, -0.25 }, -220, -110, -3.2 },  // bottom-left
        { 5, 0, { -0.29,  0.08 }, -330,   80,  2.2 },  // left
    };

    // ---- Small debris triangles (tiny, fast, disappear quickly) ----
    struct DebrisDef { Math::vec2 c; double vx, vy, angVel; };
    constexpr DebrisDef Debris[6] = {
        { { -0.42,  0.45 }, -380, 260, -9.0 },
        { {  0.40,  0.40 },  400, 240,  8.5 },
        { {  0.46, -0.40 },  350,-130, -7.5 },
        { { -0.40, -0.42 }, -320,-150,  8.0 },
        { {  0.10,  0.48 },  100, 500,  6.0 },
        { { -0.05, -0.48 },  -80,-480, -6.5 },
    };

    // Transform a shard vertex (normalized local offset from centroid)
    // into world space, applying rotation.
    Math::vec2 TformVert(Math::vec2 normLocal,     // vertex in normalized space
                         Math::vec2 normCentroid,  // shard centroid in normalized space
                         Math::vec2 worldCentroid, // current world position of centroid
                         double hw, double hh,
                         double angle)
    {
        // Offset from centroid in world scale (unrotated)
        const double dx = (normLocal.x - normCentroid.x) * hw * 2.0;
        const double dy = (normLocal.y - normCentroid.y) * hh * 2.0;

        const double cosA = std::cos(angle);
        const double sinA = std::sin(angle);

        return {
            worldCentroid.x + dx * cosA - dy * sinA,
            worldCentroid.y + dx * sinA + dy * cosA
        };
    }
}

// ============================================================
//  BreakableWall
// ============================================================

BreakableWall::BreakableWall(Math::vec2 pos, Math::vec2 in_size, const std::string& id)
    : CS230::GameObject(pos), size(in_size)
{
    waterWall = (id.find("WATER") != std::string::npos);
    Math::irect box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };
    AddGOComponent(new CS230::RectCollision(box, this));
}

void BreakableWall::Update(double dt)
{
    if (state == State::Breaking)
    {
        breakTimer += dt;
        if (breakTimer >= BreakDuration)
        {
            if (waterWall)
                state = State::Broken;
            else
                Destroy();
        }
    }
    CS230::GameObject::Update(dt);
}

void BreakableWall::Draw(const Math::TransformationMatrix& /*camera_matrix*/)
{
    auto&            r   = Engine::GetRenderer2D();
    const Math::vec2 pos = GetPosition();
    const double     hw  = size.x * 0.5;
    const double     hh  = size.y * 0.5;

    // Fully broken water wall — invisible until reset
    if (state == State::Broken) return;

    // ----------------------------------------------------------------
    //  INTACT – dark tile + crack lines radiating from crack centre
    // ----------------------------------------------------------------
    if (state == State::Intact)
    {
        // Base rectangle (match dark terrain tiles)
        const auto mat = Math::TranslationMatrix(pos) * Math::ScaleMatrix(size);
        r.DrawRectangle(mat, 0x131313FF, 0x2C2C2CFF, 1.5);

        // Slightly lighter inner face for depth
        const auto innerMat = Math::TranslationMatrix(pos)
                            * Math::ScaleMatrix({ size.x - 4.0, size.y - 4.0 });
        r.DrawRectangle(innerMat, 0x1B1B1BFF, 0x00000000u, 0.0);

        // Crack lines: from crack centre to each edge point
        const Math::vec2 cc = {
            pos.x + CRACK_CENTER.x * hw * 2.0,
            pos.y + CRACK_CENTER.y * hh * 2.0
        };
        for (int i = 0; i < 6; ++i)
        {
            const Math::vec2 ep = {
                pos.x + EDGE_PTS[i].x * hw * 2.0,
                pos.y + EDGE_PTS[i].y * hh * 2.0
            };
            r.DrawLine(cc, ep, 0x505050CCu, 1.5);
            r.DrawLine(cc, ep, 0x88888844u, 0.5); // faint glow on top
        }

        // Small dot at crack centre
        const auto dotMat = Math::TranslationMatrix(cc)
                          * Math::ScaleMatrix({ 4.0, 4.0 });
        r.DrawCircle(dotMat, 0x666666AAu, 0x00000000u, 0.0);
        return;
    }

    // ----------------------------------------------------------------
    //  BREAKING – triangular shards fly outward with spin
    // ----------------------------------------------------------------
    const double t = breakTimer / BreakDuration;

    // Impact flash (first FlashDuration seconds)
    if (breakTimer < FlashDuration)
    {
        const uint32_t fa = static_cast<uint32_t>((1.0 - breakTimer / FlashDuration) * 255.0);
        const auto flashMat = Math::TranslationMatrix(pos) * Math::ScaleMatrix(size);
        r.DrawRectangle(flashMat, (0xFFFFFFu << 8) | fa, (0xFFFFFFu << 8) | fa, 0.0);
    }

    // Alpha curve: full until 35% of duration, then fade
    const double alphaNorm = t < 0.35 ? 1.0 : 1.0 - (t - 0.35) / 0.65;
    const uint32_t a       = static_cast<uint32_t>(std::max(0.0, alphaNorm) * 255.0);
    if (a == 0) return;

    const uint32_t fillCol = (0x1A1A1Au << 8) | a;
    const uint32_t edgeCol = (0x606060u << 8) | a;
    const uint32_t crackEdgeCol = (0x909090u << 8) | (a * 3 / 4);

    for (int i = 0; i < 6; ++i)
    {
        const ShardDef& s = Shards[i];

        // World centroid position (projectile motion)
        const Math::vec2 worldCentroid = {
            pos.x + s.centroid.x * hw * 2.0 + s.vx * breakTimer,
            pos.y + s.centroid.y * hh * 2.0 + s.vy * breakTimer
                  - 0.5 * Gravity * breakTimer * breakTimer
        };
        const double angle = s.angVel * breakTimer;

        // Three vertices of this shard (in normalized space)
        const Math::vec2 vA_n = CRACK_CENTER;
        const Math::vec2 vB_n = EDGE_PTS[s.ep0];
        const Math::vec2 vC_n = EDGE_PTS[s.ep1];

        // Transform to world space
        const Math::vec2 wA = TformVert(vA_n, s.centroid, worldCentroid, hw, hh, angle);
        const Math::vec2 wB = TformVert(vB_n, s.centroid, worldCentroid, hw, hh, angle);
        const Math::vec2 wC = TformVert(vC_n, s.centroid, worldCentroid, hw, hh, angle);

        // Fill shard
        FillTriangle(r, wA, wB, wC, fillCol);

        // Outline edges
        r.DrawLine(wA, wB, edgeCol, 1.5);
        r.DrawLine(wB, wC, edgeCol, 1.5);
        r.DrawLine(wC, wA, edgeCol, 1.5);

        // Bright crack-surface highlight on the two "inner" edges (where the crack cut)
        r.DrawLine(wB, wA, crackEdgeCol, 1.0);
        r.DrawLine(wA, wC, crackEdgeCol, 1.0);
    }

    // Small debris triangles (scatter quickly, fade fast)
    const double dAlphaNorm = t < 0.15 ? 1.0 : 1.0 - (t - 0.15) / 0.85;
    const uint32_t da = static_cast<uint32_t>(std::max(0.0, dAlphaNorm) * 200.0);
    if (da > 0)
    {
        const uint32_t dFill = (0x222222u << 8) | da;
        const uint32_t dEdge = (0x555555u << 8) | da;
        const double   dsz   = std::min(hw, hh) * 0.18;

        for (int i = 0; i < 6; ++i)
        {
            const DebrisDef& d = Debris[i];
            const Math::vec2 dc = {
                pos.x + d.c.x * hw * 2.0 + d.vx * breakTimer,
                pos.y + d.c.y * hh * 2.0 + d.vy * breakTimer
                      - 0.5 * Gravity * breakTimer * breakTimer
            };
            const double da_ang = d.angVel * breakTimer;
            const double cosA   = std::cos(da_ang);
            const double sinA   = std::sin(da_ang);

            // Tiny equilateral-ish triangle
            const Math::vec2 dv[3] = {
                { dc.x + dsz * cosA,                  dc.y + dsz * sinA },
                { dc.x + dsz * std::cos(da_ang + 2.1), dc.y + dsz * std::sin(da_ang + 2.1) },
                { dc.x + dsz * std::cos(da_ang + 4.2), dc.y + dsz * std::sin(da_ang + 4.2) },
            };
            FillTriangle(r, dv[0], dv[1], dv[2], dFill);
            r.DrawLine(dv[0], dv[1], dEdge, 1.0);
            r.DrawLine(dv[1], dv[2], dEdge, 1.0);
            r.DrawLine(dv[2], dv[0], dEdge, 1.0);
        }
    }
}

bool BreakableWall::CanCollideWith(GameObjectTypes other)
{
    return other == GameObjectTypes::Player;
}

void BreakableWall::ResolveCollision(CS230::GameObject* other)
{
    if (state != State::Intact) return;
    if (other->Type() != GameObjectTypes::Player) return;

    Player* player = static_cast<Player*>(other);
    if (player->dashComponent.IsDashing())
    {
        Break();
        if (waterWall)
        {
            const double dx = player->GetPosition().x - GetPosition().x;
            Math::vec2 rushDir = { dx < 0.0 ? -1.0 : 1.0, 0.0 };
            player->ApplyWaterRush(rushDir, GetPosition());
        }
    }
}

void BreakableWall::ResetIfWater()
{
    if (!waterWall || state == State::Intact) return;
    state      = State::Intact;
    breakTimer = 0.0;
    Math::irect box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };
    AddGOComponent(new CS230::RectCollision(box, this));
}

void BreakableWall::Break()
{
    if (state != State::Intact) return;
    state      = State::Breaking;
    breakTimer = 0.0;
    RemoveGOComponent<CS230::RectCollision>();
}
