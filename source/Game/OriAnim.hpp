#pragma once
#include "Engine/Matrix.hpp"
#include "Engine/Texture.hpp"
#include "Engine/Vec2.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct OriFrame {
    std::shared_ptr<CS230::Texture> texture;
    Math::ivec2                     texel;     // atlas crop origin (pixels)
    Math::ivec2                     crop;      // atlas crop size (pixels)
    int                             displayW;  // rendered width
    int                             displayH;  // rendered height
};

struct OriClip {
    std::string           name;
    std::vector<OriFrame> frames;
    float                 fps   = 30.f;
    bool                  loop  = true;
    float                 scale = 1.f;
};

class OriAnimation {
public:
    void Build();

    void               Play(const std::string& name);
    void               Update(double dt);
    void               Draw(Math::vec2 feet_pos, bool facing_right) const;
    bool               Finished() const;
    const std::string& Current() const { return current; }

private:
    void AddClip(OriClip clip);

    std::unordered_map<std::string, OriClip> clips;
    std::string current;
    float       timer = 0.f;
    int         frame = 0;
};
