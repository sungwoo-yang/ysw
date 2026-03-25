#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/Texture.hpp"
#include "GameObjectTypes.hpp" // For GameObjectTypes
#include <memory>
#include <string>

namespace CS230 {
    class BackgroundElement : public GameObject {
    public:
        BackgroundElement(Math::vec2 pos, const std::string& texturePath);

        // Matches exactly: const Math::TransformationMatrix&
        void Draw(const Math::TransformationMatrix& camera_matrix) override;
        void Update(double dt) override;

        // Must implement these pure virtual functions
        GameObjectTypes Type() override { return GameObjectTypes::Background; }
        std::string TypeName() override { return "BackgroundElement"; }

    private:
        std::shared_ptr<Texture> texturePtr;
    };
}