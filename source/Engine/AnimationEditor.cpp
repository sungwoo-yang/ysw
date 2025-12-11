#include "AnimationEditor.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "Engine/Path.hpp"
#include <imgui.h>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <cmath> // std::abs

AnimationEditor::AnimationEditor() {
}

void AnimationEditor::SetSkeleton(Skeleton* targetSkeleton) {
    skeleton = targetSkeleton;
}

void AnimationEditor::Update(double dt) {
    if (!isEditing || !skeleton) return;

    if (isPlaying) {
        currentTime += dt;
        if (currentTime > currentAnim.duration) {
            if (currentAnim.loop) {
                currentTime = 0.0;
            } else {
                currentTime = currentAnim.duration;
                isPlaying = false;
            }
        }
        ApplyKeyframe(currentTime);
    }
}

// Simple linear interpolation
double Interpolate(double t, double t1, double v1, double t2, double v2) {
    if (t2 == t1) return v1;
    double factor = (t - t1) / (t2 - t1);
    return v1 + (v2 - v1) * factor;
}

void AnimationEditor::ApplyKeyframe(double time) {
    for (auto& [name, track] : currentAnim.tracks) {
        Bone* bone = skeleton->GetBone(name);
        if (!bone || track.keyframes.empty()) continue;

        auto& keys = track.keyframes;
        Keyframe k1 = keys.front();
        Keyframe k2 = keys.back();

        bool found = false;
        for (size_t i = 0; i < keys.size() - 1; ++i) {
            if (time >= keys[i].time && time < keys[i+1].time) {
                k1 = keys[i];
                k2 = keys[i+1];
                found = true;
                break;
            }
        }

        double angle = 0.0;
        if (time <= keys.front().time) angle = keys.front().value;
        else if (time >= keys.back().time) angle = keys.back().value;
        else {
            angle = Interpolate(time, k1.time, k1.value, k2.time, k2.value);
        }

        bone->angle = angle;
    }
}

void AnimationEditor::DrawImGui() {
#ifdef DEVELOPER_VERSION
    if (ImGui::Begin("Animation Editor")) {
        
        ImGui::Checkbox("Enable Editing Mode", &isEditing);
        
        if (!skeleton) {
            ImGui::TextColored(ImVec4(1,0,0,1), "No Skeleton Attached!");
            ImGui::End();
            return;
        }

        ImGui::Separator();
        
        // 1. Playback Controls
        ImGui::Text("Timeline");
        if (ImGui::Button(isPlaying ? "Pause" : "Play")) {
            isPlaying = !isPlaying;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            isPlaying = false;
            currentTime = 0.0;
            ApplyKeyframe(0.0);
        }
        
        float time = static_cast<float>(currentTime);
        if (ImGui::SliderFloat("Time", &time, 0.0f, static_cast<float>(currentAnim.duration))) {
            currentTime = static_cast<double>(time);
            ApplyKeyframe(currentTime); 
        }
        
        float duration = static_cast<float>(currentAnim.duration);
        if (ImGui::InputFloat("Duration", &duration)) {
            currentAnim.duration = static_cast<double>(duration);
        }
        ImGui::Checkbox("Loop", &currentAnim.loop);

        ImGui::Separator();

        // 2. Bone Selection & Editing
        ImGui::Text("Bone Controls");
        
        if (ImGui::BeginCombo("Select Bone", selectedBoneName.c_str())) {
            std::vector<std::string> boneList = { 
                "Hips", "SpineLower", "SpineUpper", "Neck", "Head",
                "L_Thigh", "L_Calf", "R_Thigh", "R_Calf",
                "L_Clavicle", "L_Arm_Up", "L_Arm_Low",
                "R_Clavicle", "R_Arm_Up", "R_Arm_Low" 
            };
            
            for (const auto& name : boneList) {
                bool isSelected = (selectedBoneName == name);
                if (ImGui::Selectable(name.c_str(), isSelected)) {
                    selectedBoneName = name;
                    selectedBone = skeleton->GetBone(name);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (selectedBone) {
            float angleDeg = static_cast<float>(selectedBone->angle * 180.0 / 3.1415926535);
            if (ImGui::DragFloat("Angle (Deg)", &angleDeg, 1.0f, -180.0f, 180.0f)) {
                selectedBone->angle = static_cast<double>(angleDeg * 3.1415926535 / 180.0);
            }

            if (ImGui::Button("Add Keyframe")) {
                BoneTrack& track = currentAnim.tracks[selectedBoneName];
                track.boneName = selectedBoneName;
                
                bool updated = false;
                for (auto& k : track.keyframes) {
                    if (std::abs(k.time - currentTime) < 0.01) {
                        k.value = selectedBone->angle;
                        updated = true;
                        break;
                    }
                }
                if (!updated) {
                    track.keyframes.push_back({ currentTime, selectedBone->angle });
                    std::sort(track.keyframes.begin(), track.keyframes.end(), 
                        [](const Keyframe& a, const Keyframe& b){ return a.time < b.time; });
                }
            }
        }

        ImGui::Separator();

        // 3. File I/O
        ImGui::InputText("Anim Name", animNameBuffer, sizeof(animNameBuffer));
        if (ImGui::Button("Save Animation")) {
            std::string filename = std::string(animNameBuffer) + ".txt";
            SaveAnimation(filename);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Animation")) {
            std::string filename = std::string(animNameBuffer) + ".txt";
            LoadAnimation(filename);
        }
    }
    ImGui::End();
#endif
}

void AnimationEditor::SaveAnimation(const std::filesystem::path& filename) {
    std::filesystem::path fullPath = assets::get_base_path() / "Animations" / filename;
    
    std::filesystem::create_directories(fullPath.parent_path());

    std::ofstream out(fullPath);
    if (!out.is_open()) {
        Engine::GetLogger().LogError("Failed to save animation: " + fullPath.string());
        return;
    }

    out << "Animation: " << animNameBuffer << "\n";
    out << "Duration: " << currentAnim.duration << "\n";
    out << "Loop: " << (currentAnim.loop ? "true" : "false") << "\n";
    
    for (const auto& [boneName, track] : currentAnim.tracks) {
        if (track.keyframes.empty()) continue;
        
        out << "Track: " << boneName << "\n";
        out << "Keyframes: " << track.keyframes.size() << "\n";
        for (const auto& k : track.keyframes) {
            out << k.time << " " << k.value << "\n";
        }
    }
    
    out.close();
    Engine::GetLogger().LogEvent("Animation Saved: " + fullPath.string());
}

void AnimationEditor::LoadAnimation(const std::filesystem::path& filename) {
    std::filesystem::path fullPath = assets::get_base_path() / "Animations" / filename;
    std::ifstream in(fullPath);
    
    if (!in.is_open()) {
        Engine::GetLogger().LogError("Failed to load animation: " + fullPath.string());
        return;
    }

    currentAnim = AnimationData(); // Clear
    std::string line;
    std::string currentTrackBone = "";

    while (in >> line) {
        if (line == "Animation:") {
            std::string temp; std::getline(in, temp); 
        } else if (line == "Duration:") {
            in >> currentAnim.duration;
        } else if (line == "Loop:") {
            std::string boolStr; in >> boolStr;
            currentAnim.loop = (boolStr == "true");
        } else if (line == "Track:") {
            in >> currentTrackBone;
            currentAnim.tracks[currentTrackBone].boneName = currentTrackBone;
        } else if (line == "Keyframes:") {
            int count; in >> count;
            for (int i=0; i<count; ++i) {
                double t, v;
                in >> t >> v;
                currentAnim.tracks[currentTrackBone].keyframes.push_back({t, v});
            }
        }
    }
    in.close();
    
    std::string name = filename.stem().string();
    size_t len = std::min(name.length(), sizeof(animNameBuffer) - 1);
    std::copy(name.begin(), name.begin() + len, animNameBuffer);
    animNameBuffer[len] = '\0';

    Engine::GetLogger().LogEvent("Animation Loaded: " + fullPath.string());
}