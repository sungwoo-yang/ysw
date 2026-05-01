#include "Constellation.hpp"

#include "LaserStar.hpp"
#include "TargetStar.hpp"

Constellation::Constellation(std::string in_name) : name(std::move(in_name))
{
}

void Constellation::SetMainStar(LaserStar* star)
{
    mainStar = star;
}

void Constellation::AddTargetStar(TargetStar* star)
{
    if (star != nullptr)
    {
        targetStars.push_back(star);
    }
}

LaserStar* Constellation::GetMainStar() const
{
    return mainStar;
}

const std::vector<TargetStar*>& Constellation::GetTargetStars() const
{
    return targetStars;
}

int Constellation::GetActivatedTargetCount() const
{
    int count = 0;

    for (TargetStar* target : targetStars)
    {
        if (target != nullptr && target->IsHit())
        {
            ++count;
        }
    }

    return count;
}

int Constellation::GetTotalTargetCount() const
{
    return static_cast<int>(targetStars.size());
}

bool Constellation::IsRestored() const
{
    return !targetStars.empty() && GetActivatedTargetCount() == GetTotalTargetCount();
}

const std::string& Constellation::GetName() const
{
    return name;
}