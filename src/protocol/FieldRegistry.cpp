#include "FieldRegistry.h"

#include "common.h"
#include "CPlayerPed.h"
#include "CVector.h"
#include "CWeapon.h"
#include "CWanted.h"
#include "CWorld.h"
#include "CPlayerInfo.h"
#include "CClock.h"
#include "CRadar.h"
#include "CCamera.h"
#include "CPools.h"

#include <functional>
#include <unordered_map>

namespace {

using FieldFn = std::function<nlohmann::json()>;
std::unordered_map<std::string, FieldFn> g_fields;

static inline CPlayerPed* localPed() { return FindPlayerPed(-1); }

// ── position ──────────────────────────────────────────────────────────────────

static nlohmann::json readPosition()
{
    CPlayerPed* p = localPed(); if (!p) return nullptr;
    const CVector& v = p->GetPosition();
    return nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}
static nlohmann::json readPositionX() { CPlayerPed* p = localPed(); return p ? nlohmann::json(p->GetPosition().x) : nullptr; }
static nlohmann::json readPositionY() { CPlayerPed* p = localPed(); return p ? nlohmann::json(p->GetPosition().y) : nullptr; }
static nlohmann::json readPositionZ() { CPlayerPed* p = localPed(); return p ? nlohmann::json(p->GetPosition().z) : nullptr; }

// ── angle (heading in degrees, 0–360, clockwise from north) ──────────────────

static nlohmann::json readAngle()
{
    CPlayerPed* p = localPed(); if (!p) return nullptr;
    constexpr float kRad2Deg = 180.0f / 3.14159265358979f;
    float deg = p->m_fHeadingCurrent * kRad2Deg;
    // Normalise to [0, 360)
    while (deg <   0.0f) deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;
    return deg;
}

// ── camera_angle (camera heading used for minimap rotation, degrees, 0–360) ──

static nlohmann::json readCameraAngle()
{
    constexpr float kRad2Deg = 180.0f / 3.14159265358979f;
    float deg = CRadar::m_fRadarOrientation * kRad2Deg;
    // Normalise to [0, 360)
    while (deg <   0.0f) deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;
    return deg;
}

// ── vitals ────────────────────────────────────────────────────────────────────

static nlohmann::json readHealth() { CPlayerPed* p = localPed(); return p ? nlohmann::json(p->m_fHealth) : nullptr; }
static nlohmann::json readArmour() { CPlayerPed* p = localPed(); return p ? nlohmann::json(p->m_fArmour) : nullptr; }

// ── economy & law ─────────────────────────────────────────────────────────────

static nlohmann::json readMoney()
{
    if (!CWorld::Players) return 0;
    return nlohmann::json(CWorld::Players[0].m_nMoney);
}

static nlohmann::json readWanted()
{
    CPlayerPed* p = localPed(); if (!p) return 0;
    CWanted* w = p->GetWanted(); if (!w) return 0;
    int stars = static_cast<int>(w->m_nWantedLevel / 100 / 16);
    if (stars > 6) stars = 6;
    return stars;
}

static nlohmann::json readWantedRaw()
{
    CPlayerPed* p = localPed(); if (!p) return 0;
    CWanted* w = p->GetWanted(); if (!w) return 0;
    return static_cast<int>(w->m_nWantedLevel);
}

// ── ped state ─────────────────────────────────────────────────────────────────

static nlohmann::json readPedState()  { CPlayerPed* p = localPed(); return p ? nlohmann::json(static_cast<int>(p->m_ePedState))  : nullptr; }
static nlohmann::json readMoveState() { CPlayerPed* p = localPed(); return p ? nlohmann::json(static_cast<int>(p->m_nMoveState)) : nullptr; }
static nlohmann::json readInVehicle() { CPlayerPed* p = localPed(); return p ? nlohmann::json((bool)p->bInVehicle)               : nullptr; }
static nlohmann::json readAreaCode()  { CPlayerPed* p = localPed(); return p ? nlohmann::json((int)p->m_nAreaCode)               : nullptr; }

// ── weapon ────────────────────────────────────────────────────────────────────

static nlohmann::json readWeaponSlot()
{
    CPlayerPed* p = localPed(); if (!p) return nullptr;
    return static_cast<int>(p->m_nSelectedWepSlot);
}
static nlohmann::json readCurrentWeapon()
{
    CPlayerPed* p = localPed(); if (!p) return nullptr;
    return static_cast<int>(p->GetWeapon()->m_eWeaponType);
}
static nlohmann::json readAmmoClip()
{
    CPlayerPed* p = localPed(); if (!p) return nullptr;
    return static_cast<int>(p->GetWeapon()->m_nAmmoInClip);
}
static nlohmann::json readAmmoTotal()
{
    CPlayerPed* p = localPed(); if (!p) return nullptr;
    return static_cast<int>(p->GetWeapon()->m_nAmmoTotal);
}

// ── game time ─────────────────────────────────────────────────────────────────

static nlohmann::json readGameTime()
{
    return nlohmann::json{{"hour",   (int)CClock::ms_nGameClockHours},
                          {"minute", (int)CClock::ms_nGameClockMinutes}};
}
static nlohmann::json readGameHour()   { return (int)CClock::ms_nGameClockHours;   }
static nlohmann::json readGameMinute() { return (int)CClock::ms_nGameClockMinutes; }

// ── map blips ─────────────────────────────────────────────────────────────────

// System sprites that are drawn separately from the main blip array.
// They should not appear in the blips data feed.
static bool isSystemSprite(int sprite)
{
    return sprite == RADAR_SPRITE_CENTRE  ||  // 2 — map centre crosshair
           sprite == RADAR_SPRITE_MAP_HERE ||  // 3 — "you are here" arrow
           sprite == RADAR_SPRITE_NORTH;       // 4 — north indicator
}

// Returns true if the entity referenced by an entity blip still exists.
// Entity blips (BLIP_CAR, BLIP_CHAR, BLIP_OBJECT, BLIP_PICKUP) depend on
// live game objects; if the object is gone the blip becomes stale.
static bool entityHandleValid(unsigned int handle, int type)
{
    switch (type) {
    case BLIP_CAR:    return CPools::GetVehicle((int)handle) != nullptr;
    case BLIP_CHAR:   return CPools::GetPed((int)handle) != nullptr;
    case BLIP_OBJECT: return CPools::GetObject((int)handle) != nullptr;
    // BLIP_PICKUP uses the pickup pool (CPickups), not the object pool.
    // We skip validation for pickups since CPickups doesn't expose a
    // public handle-lookup in the plugin-sdk. The game still draws
    // stale pickup blips with m_bBlipRemain semantics.
    default:          return true; // coordinate blips have no entity handle
    }
}

static nlohmann::json readBlips()
{
    nlohmann::json arr = nlohmann::json::array();
    if (!CRadar::ms_RadarTrace) return arr;

    const unsigned int count = MAX_RADAR_TRACES; // 175

    // Cache player position for short-range distance checks
    CPlayerPed* player = FindPlayerPed(-1);
    CVector playerPos;
    bool hasPlayerPos = (player != nullptr);
    if (hasPlayerPos)
        playerPos = player->GetPosition();

    for (unsigned int i = 0; i < count; ++i) {
        const tRadarTrace& t = CRadar::ms_RadarTrace[i];

        // ── basic slot validity (matches game's DrawBlips logic) ────────
        if (!t.m_bInUse || t.m_nBlipType == BLIP_NONE)
            continue;

        // ── hidden blip (the game does not draw these at all) ───────────
        if (t.m_nBlipDisplay == BLIP_DISPLAY_NEITHER)
            continue;

        // ── system sprites (drawn outside the main blip loop) ───────────
        if (isSystemSprite(t.m_nRadarSprite))
            continue;

        // ── entity blips: skip if the entity is gone and blip won't remain ──
        const bool isEntityBlip = (t.m_nBlipType == BLIP_CAR  ||
                                   t.m_nBlipType == BLIP_CHAR ||
                                   t.m_nBlipType == BLIP_OBJECT ||
                                   t.m_nBlipType == BLIP_PICKUP);
        if (isEntityBlip) {
            if (!entityHandleValid(t.m_nEntityHandle, t.m_nBlipType)) {
                // Entity is gone. If m_bBlipRemain is false, the game
                // clears this blip — so we skip it here too.
                if (!t.m_bBlipRemain)
                    continue;
                // If m_bBlipRemain is true the blip persists as a "ghost"
                // marker even after the entity is deleted. We let it pass
                // but the client can use the "remain" field to identify it.
            }
        }

        arr.push_back({
            {"idx",         (int)i},
            {"type",        (int)t.m_nBlipType},
            {"sprite",      (int)t.m_nRadarSprite},
            {"display",     (int)t.m_nBlipDisplay},
            {"color",       (int)t.m_nColour},
            {"x",           t.m_vecPos.x},
            {"y",           t.m_vecPos.y},
            {"z",           t.m_vecPos.z},
            {"size",        (int)t.m_nBlipSize},
            {"short_range", (bool)t.m_bShortRange},
            {"friendly",    (bool)t.m_bFriendly},
            {"remain",      (bool)t.m_bBlipRemain}
        });
    }
    return arr;
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

        // angle
        g_fields["angle"] = readAngle;

        // camera_angle
        g_fields["camera_angle"] = readCameraAngle;

        // vitals
        g_fields["health"] = readHealth;
        g_fields["armour"] = readArmour;

        // economy & law
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

        // map blips
        g_fields["blips"] = readBlips;
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
