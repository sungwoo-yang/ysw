#include "WhiteLaser.hpp"

WhiteLaser::WhiteLaser(Math::vec2 in_startPos, Math::vec2 in_dir, Player* in_player, const std::vector<TargetStar*>& in_targets) : Laser(in_startPos, in_dir, in_player, in_targets)
{
    // 흰색 레이저 설정
    color = 0xFFFFFFFF;
}

void WhiteLaser::Update(double dt)
{
    // 1. 매 프레임마다 레이저의 반사 경로를 다시 계산합니다.
    CalculatePath(maxBounces, maxLength);

    // 2. 계산된 경로를 바탕으로 퍼즐 타겟(TargetStar)과 충돌했는지 검사합니다.
    CheckTargetIntersections(hitRadius);

    // 참고: 플레이어에게 데미지를 입히는 코드는 의도적으로 제외되었습니다.
}