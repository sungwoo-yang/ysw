/*
 * Skeleton.hpp
 */
#pragma once
#include "Engine/Component.hpp"
#include "Bone.hpp"
#include "Engine/Matrix.hpp"
#include <map>
#include <string>

namespace CS230 { class GameObject; }

class Skeleton : public CS230::Component {
public:
    Skeleton(CS230::GameObject* owner);
    ~Skeleton();

    void Update(double dt) override;
    
    void Draw(const Math::TransformationMatrix& cameraMatrix);

    Bone* AddBone(std::string name, double length, Math::vec2 position, double angle, std::string parentName = "");
    Bone* GetBone(const std::string& name);
    Bone* GetRoot() const { return rootBone; }


private:
    CS230::GameObject* owner;
    std::map<std::string, Bone*> bones;
    Bone* rootBone = nullptr;
};