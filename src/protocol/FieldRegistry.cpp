#include "FieldRegistry.h"

#include "common.h"
#include "CPlayerPed.h"
#include "CPlayerInfo.h"
#include "CWanted.h"
#include "CWorld.h"
#include "CClock.h"
#include "CVector.h"

#include <functional>
#include <unordered_map>

namespace {

using FieldFn = std::function<nlohmann::json()>;
std::unordered_map<std::string, FieldFn> g_fields;

// Helper: returns nullptr if player unavailable
inline CPlayerPed* localPed()
{
    return FindPlayerPed(-1);
}

// ── player base ──────────────────────────────────────────────────────────────

static nlohmann::json readHealth()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(ped->m_fHealth) : nlohmann::json(nullptr);
}

static nlohmann::json readArmour()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(ped->m_fArmour) : nlohmann::json(nullptr);
}

// ── position ─────────────────────────────────────────────────────────────────

static nlohmann::json readPosition()
{
    auto* ped = localPed();
    if (!ped) return nullptr;
    const CVector& pos = ped->GetPosition();
    return nlohmann::json{{"x", pos.x}, {"y", pos.y}, {"z", pos.z}};
}

static nlohmann::json readPositionX()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(ped->GetPosition().x) : nlohmann::json(nullptr);
}

static nlohmann::json readPositionY()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(ped->GetPosition().y) : nlohmann::json(nullptr);
}

static nlohmann::json readPositionZ()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(ped->GetPosition().z) : nlohmann::json(nullptr);
}

// ── wanted ───────────────────────────────────────────────────────────────────

static nlohmann::json readWanted()
{
    auto* ped = localPed();
    if (!ped) return nullptr;
    auto* wanted = ped->GetWanted();
    if (!wanted) return 0;
    // Convert raw level to star count (1 star ≈ 100 units, step 100)
    return static_cast<int>(wanted->m_nWantedLevel / 100);
}

static nlohmann::json readWantedRaw()
{
    auto* ped = localPed();
    if (!ped) return nullptr;
    auto* wanted = ped->GetWanted();
    return wanted ? nlohmann::json(wanted->m_nWantedLevel) : nlohmann::json(0);
}

// ── money ────────────────────────────────────────────────────────────────────

static nlohmann::json readMoney()
{
    if (!CWorld::Players) return nullptr;
    return CWorld::Players[0].m_nMoney;
}

// ── ped state ────────────────────────────────────────────────────────────────

static nlohmann::json readPedState()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(static_cast<int>(ped->m_ePedState)) : nlohmann::json(nullptr);
}

static nlohmann::json readMoveState()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(static_cast<int>(ped->m_nMoveState)) : nlohmann::json(nullptr);
}

static nlohmann::json readInVehicle()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(static_cast<bool>(ped->bInVehicle)) : nlohmann::json(nullptr);
}

// ── weapon ───────────────────────────────────────────────────────────────────

static nlohmann::json readCurrentWeapon()
{
    auto* ped = localPed();
    if (!ped) return nullptr;
    auto* wpn = ped->GetWeapon();
    if (!wpn) return nullptr;
    return static_cast<int>(wpn->m_eWeaponType);
}

static nlohmann::json readWeaponSlot()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(static_cast<int>(ped->m_nSelectedWepSlot)) : nlohmann::json(nullptr);
}

static nlohmann::json readAmmoClip()
{
    auto* ped = localPed();
    if (!ped) return nullptr;
    auto* wpn = ped->GetWeapon();
    return wpn ? nlohmann::json(static_cast<unsigned int>(wpn->m_nAmmoInClip)) : nlohmann::json(0);
}

static nlohmann::json readAmmoTotal()
{
    auto* ped = localPed();
    if (!ped) return nullptr;
    auto* wpn = ped->GetWeapon();
    return wpn ? nlohmann::json(static_cast<unsigned int>(wpn->m_nAmmoTotal)) : nlohmann::json(0);
}

// ── game time ────────────────────────────────────────────────────────────────

static nlohmann::json readGameTime()
{
    return nlohmann::json{
        {"hour",   static_cast<int>(CClock::ms_nGameClockHours)},
        {"minute", static_cast<int>(CClock::ms_nGameClockMinutes)}
    };
}

static nlohmann::json readGameHour()
{
    return static_cast<int>(CClock::ms_nGameClockHours);
}

static nlohmann::json readGameMinute()
{
    return static_cast<int>(CClock::ms_nGameClockMinutes);
}

// ── area ─────────────────────────────────────────────────────────────────────

static nlohmann::json readAreaCode()
{
    auto* ped = localPed();
    return ped ? nlohmann::json(static_cast<int>(ped->m_nAreaCode)) : nlohmann::json(nullptr);
}

} // anonymous namespace

namespace FieldRegistry {

    void init()
    {
        // position
        g_fields["position"]   = readPosition;
        g_fields["position_x"] = readPositionX;
        g_fields["position_y"] = readPositionY;
        g_fields["position_z"] = readPositionZ;

        // vitals
        g_fields["health"]     = readHealth;
        g_fields["armour"]     = readArmour;

        // economy / law
        g_fields["money"]      = readMoney;
        g_fields["wanted"]     = readWanted;
        g_fields["wanted_raw"] = readWantedRaw;

        // ped state
        g_fields["ped_state"]  = readPedState;
        g_fields["move_state"] = readMoveState;
        g_fields["in_vehicle"] = readInVehicle;
        g_fields["area_code"]  = readAreaCode;

        // weapon
        g_fields["current_weapon"] = readCurrentWeapon;
        g_fields["weapon_slot"]    = readWeaponSlot;
        g_fields["ammo_clip"]      = readAmmoClip;
        g_fields["ammo_total"]     = readAmmoTotal;

        // game time
        g_fields["game_time"]   = readGameTime;
        g_fields["game_hour"]   = readGameHour;
        g_fields["game_minute"] = readGameMinute;
    }

    bool has(const std::string& name)
    {
        return g_fields.count(name) != 0;
    }

    nlohmann::json get(const std::string& name)
    {
        auto it = g_fields.find(name);
        if (it == g_fields.end()) return nullptr;
        return it->second();
    }

} // namespace FieldRegistry
