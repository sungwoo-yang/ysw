/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "GameObjectManager.hpp"
#include "Engine.hpp"
#include "Logger.hpp"

namespace CS230
{

    void GameObjectManager::Add(GameObject* object)
    {
        objects.push_back(object);
    }

    void GameObjectManager::Unload()
    {
        for (auto object : objects)
        {
            delete object;
        }
        objects.clear();
    }

    void GameObjectManager::UpdateAll(double dt)
    {
        std::vector<GameObject*> destroy_objects;

        for (auto object : objects)
        {
            object->Update(dt);

            if (object->Destroyed())
            {
                destroy_objects.push_back(object);
            }
        }

        for (GameObject* obj : destroy_objects)
        {
            objects.remove(obj);
            delete obj;
        }
    }

    void GameObjectManager::DrawAll(Math::TransformationMatrix camera_matrix)
    {
        for (auto object : objects)
        {
            object->Draw(camera_matrix);
        }
    }

    void GameObjectManager::CollisionTest()
    {
        for (GameObject* object_1 : objects)
        {
            for (GameObject* object_2 : objects)
            {
                if ((object_1 != object_2) && (object_1->CanCollideWith(object_2->Type())))
                {
                    if (object_1->IsCollidingWith(object_2))
                    {
                        Engine::GetLogger().LogEvent("Collision Detected: " + object_1->TypeName() + " and " + object_2->TypeName());
                        object_1->ResolveCollision(object_2);
                    }
                }
            }
        }
    }

    void GameObjectManager::DrawAllImGui()
    {
        for (auto object : objects)
        {
            object->DrawImGui();
        }
    }
}