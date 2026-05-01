#pragma once

#include <string>
#include <vector>

class LaserStar;
class TargetStar;

class Constellation
{
public:
    explicit Constellation(std::string in_name);

    void SetMainStar(LaserStar* star);
    void AddTargetStar(TargetStar* star);

    LaserStar*                      GetMainStar() const;
    const std::vector<TargetStar*>& GetTargetStars() const;

    int GetActivatedTargetCount() const;
    int GetTotalTargetCount() const;

    bool IsRestored() const;

    const std::string& GetName() const;

private:
    std::string name;

    LaserStar*               mainStar = nullptr;
    std::vector<TargetStar*> targetStars;
};