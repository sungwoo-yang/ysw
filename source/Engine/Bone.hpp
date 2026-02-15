/*
 * Bone.hpp
 */
#pragma once
#include "Engine/Matrix.hpp"
#include "Engine/Vec2.hpp"
#include <cmath>
#include <string>
#include <vector>

// Represents a single joint/bone in a hierarchical skeletal animation system
struct Bone
{
    std::string        name;
    Bone*              parent = nullptr;
    std::vector<Bone*> children;

    // Local transform properties relative to the parent bone
    Math::vec2 localPosition;
    double     angle;
    double     length;

    // Cached world-space transform and positions calculated during Update
    Math::TransformationMatrix worldMatrix;
    Math::vec2                 worldStartPos;
    Math::vec2                 worldEndPos;

    // Initializes bone with local transform and physical properties
    Bone(const std::string& in_name, double in_length, Math::vec2 in_pos, double in_angle) : name(in_name), localPosition(in_pos), angle(in_angle), length(in_length)
    {
    }

    // Recursively updates world transforms based on parent's matrix (Forward Kinematics)
    void Update(const Math::TransformationMatrix& parentMatrix)
    {
        // Calculate local transformation (Translate -> Rotate)
        Math::TransformationMatrix localMatrix = Math::TranslationMatrix(localPosition) * Math::RotationMatrix(angle);

        // Combine with parent to get absolute world transformation
        worldMatrix = parentMatrix * localMatrix;

        // Start position is the origin of the bone in world space
        worldStartPos = worldMatrix * Math::vec2{ 0.0, 0.0 };

        // End position is calculated based on the bone's length along its local -Y axis
        Math::vec2 tipOffset = { 0.0, -length };
        worldEndPos          = worldMatrix * tipOffset;

        // Propagate transform update to all child bones
        for (auto* child : children)
        {
            child->Update(worldMatrix);
        }
    }
};