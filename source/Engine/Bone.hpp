/*
 * Bone.hpp
 */
#pragma once
#include "Engine/Vec2.hpp"
#include "Engine/Matrix.hpp"
#include <string>
#include <vector>
#include <cmath>

struct Bone {
    std::string name;
    Bone* parent = nullptr;
    std::vector<Bone*> children;

    Math::vec2 localPosition;
    double angle;
    double length;

    Math::TransformationMatrix worldMatrix;
    Math::vec2 worldStartPos;
    Math::vec2 worldEndPos;

    Bone(const std::string& n, double len, Math::vec2 pos, double ang)
        : name(n), length(len), localPosition(pos), angle(ang) {}

    void Update(const Math::TransformationMatrix& parentMatrix) {
        Math::TransformationMatrix localMatrix =
            Math::TranslationMatrix(localPosition) * Math::RotationMatrix(angle);

        worldMatrix = parentMatrix * localMatrix;

        worldStartPos = worldMatrix * Math::vec2{ 0.0, 0.0 };

        Math::vec2 tipOffset = { 0.0, -length }; 
        worldEndPos = worldMatrix * tipOffset;

        for (auto* child : children) {
            child->Update(worldMatrix);
        }
    }
};