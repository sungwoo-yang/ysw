#pragma once
#include "Game/SaveData.hpp"
#include <optional>

class SaveManager
{
public:
    static constexpr int SlotCount = 3;

    // ---- Slot selection ----
    static void SetActiveSlot(int slot) { s_activeSlot = slot; }
    static int  GetActiveSlot()         { return s_activeSlot; }

    // ---- File I/O (uses active slot) ----
    static bool                    HasSave();
    static void                    Save(const SaveData& data);
    static std::optional<SaveData> Load();
    static void                    DeleteSave();

    // ---- Per-slot queries (for menu display) ----
    static bool                    HasSave(int slot);
    static std::optional<SaveData> Load(int slot);

    // ---- Deferred-save request (Bonfire → Mode3) ----
    static void RequestSave()  { s_pending = true; }
    static bool ConsumeSaveRequest()
    {
        if (!s_pending) return false;
        s_pending = false;
        return true;
    }

private:
    static bool s_pending;
    static int  s_activeSlot;

    static std::string SavePath(int slot);

    static void                    WriteJson(const SaveData& data, const std::string& path);
    static std::optional<SaveData> ReadJson(const std::string& path);
};
