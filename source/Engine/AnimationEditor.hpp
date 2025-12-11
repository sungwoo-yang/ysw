#pragma once

#include "Bone.hpp"
#include "Skeleton.hpp"
#include <vector>
#include <string>
#include <map>
#include <filesystem> // Required for std::filesystem

// Data structure for a single keyframe
struct Keyframe {
    double time;
    double value; // Angle
};

// Animation track for a specific bone
struct BoneTrack {
    std::string boneName;
    std::vector<Keyframe> keyframes;
};

// Complete Animation Data
struct AnimationData {
    std::string name = "NewAnim";
    double duration = 1.0;
    bool loop = true;
    std::map<std::string, BoneTrack> tracks;
};

class AnimationEditor {
public:
    AnimationEditor();
    
    void SetSkeleton(Skeleton* targetSkeleton);
    void Update(double dt);
    void DrawImGui();

    bool IsEditing() const { return isEditing; }

private:
    void SaveAnimation(const std::filesystem::path& path);
    void LoadAnimation(const std::filesystem::path& path);
    void ApplyKeyframe(double time);

    Skeleton* skeleton = nullptr;
    AnimationData currentAnim;
    
    // Editor State
    bool isEditing = false;
    bool isPlaying = false;
    double currentTime = 0.0;
    
    // Selection
    Bone* selectedBone = nullptr;
    std::string selectedBoneName = "";

    // GUI buffers
    char animNameBuffer[128] = "NewAnim";
};