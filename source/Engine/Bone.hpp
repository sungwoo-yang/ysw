/*
 * Bone.hpp
 */
#pragma once
#include "Engine/Matrix.hpp"
#include "Engine/Vec2.hpp"
#include <cmath>
#include <string>
#include <vector>

struct Bone
{
    std::string        name;
    Bone*              parent = nullptr;
    std::vector<Bone*> children;

    Math::vec2 localPosition;
    double     angle;
    double     length;

    Math::TransformationMatrix worldMatrix;
    Math::vec2                 worldStartPos;
    Math::vec2                 worldEndPos;

    Bone(const std::string& in_name, double in_length, Math::vec2 in_pos, double in_angle) : name(in_name), localPosition(in_pos), angle(in_angle), length(in_length)
    {
    }

    void Update(const Math::TransformationMatrix& parentMatrix)
    {
        Math::TransformationMatrix localMatrix = Math::TranslationMatrix(localPosition) * Math::RotationMatrix(angle);

        worldMatrix = parentMatrix * localMatrix;

        worldStartPos = worldMatrix * Math::vec2{ 0.0, 0.0 };

        Math::vec2 tipOffset = { 0.0, -length };
        worldEndPos          = worldMatrix * tipOffset;

        for (auto* child : children)
        {
            child->Update(worldMatrix);
        }
    }
};