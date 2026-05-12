/*
 * Skeleton.cpp
 */
#include "Skeleton.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/Engine.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Logger.hpp"

Skeleton::Skeleton(CS230::GameObject* obj) : owner(obj) {
}

Skeleton::~Skeleton() {
    for (auto& pair : bones) {
        delete pair.second;
    }
    bones.clear();
}

Bone* Skeleton::AddBone(std::string name, double length, Math::vec2 position, double angle, std::string parentName) {
    if (bones.count(name)) {
        CS230::Logger& logger = Engine::GetLogger();
        logger.LogError("Bone already exists: " + name);
        return bones[name];
    }

    Bone* newBone = new Bone(name, length, position, angle);
    bones[name] = newBone;

    if (parentName.empty()) {
        if (rootBone != nullptr) {
            Engine::GetLogger().LogEvent("Warning: Replacing root bone with " + name);
        }
        rootBone = newBone;
    }
    else {
        if (bones.count(parentName)) {
            Bone* parent = bones[parentName];
            newBone->parent = parent;
            parent->children.push_back(newBone);
        } else {
            Engine::GetLogger().LogError("Parent bone not found: " + parentName + " for " + name);
        }
    }
    return newBone;
}

Bone* Skeleton::GetBone(const std::string& name) {
    if (bones.count(name)) {
        return bones[name];
    }
    return nullptr;
}

void Skeleton::Update([[maybe_unused]] double dt) {
    if (!rootBone || !owner) return;

    Math::TransformationMatrix ownerMatrix = owner->GetMatrix();
    rootBone->Update(ownerMatrix);
}

void Skeleton::Draw([[maybe_unused]] const Math::TransformationMatrix& cameraMatrix) {
    auto& renderer = Engine::GetRenderer2D();

    for (auto& pair : bones) {
        Bone* b = pair.second;
        
        renderer.DrawLine(b->worldStartPos, b->worldEndPos, CS200::WHITE, 2.0);

        Math::TransformationMatrix circleTransform = 
            Math::TranslationMatrix(b->worldStartPos) * Math::ScaleMatrix({2.5, 2.5});
        
        renderer.DrawCircle(circleTransform, CS200::WHITE);
    }
}