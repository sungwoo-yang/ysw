#include "SaveManager.hpp"

#include "Engine/Path.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

bool SaveManager::s_pending    = false;
int  SaveManager::s_activeSlot = 0;

std::string SaveManager::SavePath(int slot)
{
    return (assets::get_base_path() / ("Assets/save/save" + std::to_string(slot) + ".json")).string();
}

// ---- Active-slot wrappers ----

bool SaveManager::HasSave()
{
    return HasSave(s_activeSlot);
}

bool SaveManager::HasSave(int slot)
{
    return std::filesystem::exists(SavePath(slot));
}

void SaveManager::DeleteSave()
{
    std::filesystem::remove(SavePath(s_activeSlot));
}

void SaveManager::Save(const SaveData& data)
{
    const auto dir = assets::get_base_path() / "Assets/save";
    std::filesystem::create_directories(dir);
    WriteJson(data, SavePath(s_activeSlot));
}

std::optional<SaveData> SaveManager::Load()
{
    return Load(s_activeSlot);
}

std::optional<SaveData> SaveManager::Load(int slot)
{
    return ReadJson(SavePath(slot));
}

// ---------------------------------------------------------------------------
// JSON writer
// ---------------------------------------------------------------------------

void SaveManager::WriteJson(const SaveData& data, const std::string& path)
{
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "{\n";
    f << "  \"spawn_x\": "    << data.spawnX    << ",\n";
    f << "  \"spawn_y\": "    << data.spawnY    << ",\n";
    f << "  \"hp\": "         << data.hp        << ",\n";
    f << "  \"dash\": "       << (data.dash      ? "true" : "false") << ",\n";
    f << "  \"wall_climb\": " << (data.wallClimb ? "true" : "false") << ",\n";
    f << "  \"bash\": "       << (data.bash      ? "true" : "false") << ",\n";

    f << "  \"open_gates\": [";
    for (size_t i = 0; i < data.openGates.size(); ++i)
    {
        f << "\"" << data.openGates[i] << "\"";
        if (i + 1 < data.openGates.size()) f << ", ";
    }
    f << "]\n";
    f << "}\n";
}

// ---------------------------------------------------------------------------
// JSON reader
// ---------------------------------------------------------------------------

namespace
{
    std::string Trim(const std::string& s)
    {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return {};
        size_t b = s.find_last_not_of(" \t\r\n,");
        return s.substr(a, b - a + 1);
    }

    std::string Unquote(std::string s)
    {
        s = Trim(s);
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.size() - 2);
        return s;
    }

    bool ParseBool(const std::string& s) { return Trim(s) == "true"; }

    std::vector<std::string> ParseStringArray(const std::string& line)
    {
        std::vector<std::string> result;
        const size_t lb = line.find('[');
        const size_t rb = line.rfind(']');
        if (lb == std::string::npos || rb == std::string::npos || rb <= lb) return result;

        const std::string inner = line.substr(lb + 1, rb - lb - 1);
        std::istringstream ss(inner);
        std::string tok;
        while (std::getline(ss, tok, ','))
            result.push_back(Unquote(tok));
        return result;
    }
}

std::optional<SaveData> SaveManager::ReadJson(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return std::nullopt;

    SaveData data;
    std::string line;

    while (std::getline(f, line))
    {
        line = Trim(line);
        if (line.empty() || line == "{" || line == "}") continue;

        const size_t colon = line.find("\": ");
        if (colon == std::string::npos) continue;

        const std::string key   = Unquote(line.substr(0, colon + 1));
        const std::string value = line.substr(colon + 3);

        if      (key == "spawn_x")    data.spawnX    = std::stod(Trim(value));
        else if (key == "spawn_y")    data.spawnY    = std::stod(Trim(value));
        else if (key == "hp")         data.hp        = std::stod(Trim(value));
        else if (key == "dash")       data.dash      = ParseBool(value);
        else if (key == "wall_climb") data.wallClimb = ParseBool(value);
        else if (key == "bash")       data.bash      = ParseBool(value);
        else if (key == "open_gates") data.openGates = ParseStringArray(value);
    }

    return data;
}
