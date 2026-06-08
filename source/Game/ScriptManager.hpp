#pragma once
#include "Engine/Component.hpp"
#include "Game/TutorialScript.hpp"
#include <string>
#include <vector>

class CutscenePlayer;

// Runtime trigger manager.
// - CheckTriggers : called every frame from Mode3::Update
// - LoadTriggers  : loads Assets/scripts/triggers.txt
// - LoadScriptFile: reads Assets/scripts/<name>.txt -> vector<ScriptEvent>
// - SaveScriptFile: writes Assets/scripts/<name>.txt
class ScriptManager : public CS230::Component
{
public:
    void SetCutscenePlayer(CutscenePlayer* cp) { cutscenePlayer = cp; }
    void Update([[maybe_unused]] double dt) override {}

    void SetTriggers(std::vector<ScriptTrigger> ts) { triggers = std::move(ts); }
    const std::vector<ScriptTrigger>& GetTriggers() const { return triggers; }

    void CheckTriggers(Math::vec2 playerPos);

    void LoadTriggers();
    void SaveTriggers();

    static std::vector<ScriptEvent> LoadScriptFile(const std::string& name);
    static void                     SaveScriptFile(const std::string& name,
                                                   const std::vector<ScriptEvent>& events);

private:
    std::vector<ScriptTrigger> triggers;
    CutscenePlayer*            cutscenePlayer = nullptr;
};
