#include "Engine/BackgroundElement.hpp"
#include "Engine/Engine.hpp"
#include "Engine/TextureManager.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Texture.hpp"

namespace CS230 {
    BackgroundElement::BackgroundElement(Math::vec2 pos, const std::string& texturePath) 
        : GameObject(pos), texturePtr(nullptr) {
        texturePtr = Engine::GetTextureManager().Load(texturePath);
        
        // Ensure scale is initialized to 1.0
        SetScale({ 500.0, 500.0 });
    }

    void BackgroundElement::Draw(const Math::TransformationMatrix& camera_matrix) {
        if (texturePtr) {
            Math::ivec2 imgSize = texturePtr->GetSize();
            
            // Texture::Draw already applies (Scale by ImageSize) and (Translate by ImageSize/2).
            // To make the image CENTERED at GameObject's position, 
            // we need to offset the matrix by -ImageSize/2 before passing it.
            Math::vec2 centerOffset = { -imgSize.x * 0.5, -imgSize.y * 0.5 };

            // Construct World Matrix: Translation(pos) * Scale(objScale) * Translation(offset)
            // Use GetScale() instead of local 'scale' variable to avoid C4458 error
            Math::TransformationMatrix worldMatrix = 
                Math::TranslationMatrix(GetPosition()) * Math::ScaleMatrix(GetScale()) * Math::TranslationMatrix(centerOffset);

            // Final: Camera * World
            texturePtr->Draw(camera_matrix * worldMatrix);
        }
    }

    void BackgroundElement::Update(double /*dt*/) { }
}