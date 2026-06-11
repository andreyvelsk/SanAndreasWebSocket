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
#include "CPickups.h"
#include "CEntryExit.h"

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
static nlohmann::json readIsInterior(){ CPlayerPed* p = localPed(); return p ? nlohmann::json((bool)(p->m_nAreaCode != 0))       : nullptr; }

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

static nlohmann::json readBlips()
{
    nlohmann::json arr = nlohmann::json::array();
    if (!CRadar::ms_RadarTrace) return arr;

    const unsigned int count = MAX_RADAR_TRACES; // 175

    for (unsigned int i = 0; i < count; ++i) {
        const tRadarTrace& t = CRadar::ms_RadarTrace[i];

        // ── basic slot validity ────────
        if (!t.m_bInUse || t.m_nBlipType == BLIP_NONE)
            continue;

        // ── system sprites (drawn outside the main blip loop) ───────────
        if (isSystemSprite(t.m_nRadarSprite))
            continue;

        // ── resolve entity position ────
        // The game's DrawEntityBlip reads the entity position directly
        // from the live entity, not from tRadarTrace.m_vecPos. We
        // replicate that logic: for entity blips, read GetPosition()
        // from the actual entity; fall back to m_vecPos if the entity
        // is gone but m_bBlipRemain keeps the blip alive.
        CVector pos = t.m_vecPos;
        bool entityGone = false;

        switch (t.m_nBlipType) {
        case BLIP_CAR: {
            CVehicle* v = CPools::GetVehicle((int)t.m_nEntityHandle);
            if (v) pos = v->GetPosition();
            else entityGone = true;
            break;
        }
        case BLIP_CHAR: {
            CPed* ped = CPools::GetPed((int)t.m_nEntityHandle);
            if (ped) pos = ped->GetPosition();
            else entityGone = true;
            break;
        }
        case BLIP_OBJECT: {
            CObject* obj = CPools::GetObject((int)t.m_nEntityHandle);
            if (obj) pos = obj->GetPosition();
            else entityGone = true;
            break;
        }
        case BLIP_PICKUP: {
            int idx = (int)t.m_nEntityHandle;
            if (idx >= 0 && idx < MAX_NUM_PICKUPS && CPickups::aPickUps) {
                CPickup& pickup = CPickups::aPickUps[idx];
                if (pickup.m_nPickupType != PICKUP_NONE)
                    pos = pickup.GetPosn();
                else
                    entityGone = true;
            } else {
                entityGone = true;
            }
            break;
        }
        }

        // ── entity gone with no persistence: skip ──
        if (entityGone && !t.m_bBlipRemain)
            continue;

        // ── hidden blip ────────────────
        if (t.m_nBlipDisplay == BLIP_DISPLAY_NEITHER)
            continue;

        // ── interior blip: resolve entrance position ──
        // Blips with m_pEntryExit represent interior locations (save houses,
        // barbershops, restaurants, etc.). Their m_vecPos is the interior
        // room position (far outside the normal world map). The game draws
        // these blips at the entrance door position, so we compute it here:
        //   entrance_xy = center of m_recEntrance rect
        //   entrance_z  = m_fEntranceZ
        bool isInterior = false;
        CVector interiorPos; // original m_vecPos for interiors
        if (t.m_pEntryExit) {
            isInterior = true;
            interiorPos = pos; // save original (interior) position
            const CRect& r = t.m_pEntryExit->m_recEntrance;
            pos.x = (r.left + r.right) * 0.5f;
            pos.y = (r.top + r.bottom) * 0.5f;
            pos.z = t.m_pEntryExit->m_fEntranceZ;
        }

        nlohmann::json entry{
            {"idx",         (int)i},
            {"type",        (int)t.m_nBlipType},
            {"sprite",      (int)t.m_nRadarSprite},
            {"display",     (int)t.m_nBlipDisplay},
            {"color",       (int)t.m_nColour},
            {"x",           pos.x},
            {"y",           pos.y},
            {"z",           pos.z},
            {"size",        (int)t.m_nBlipSize},
            {"short_range", (bool)t.m_bShortRange},
            {"friendly",    (bool)t.m_bFriendly},
            {"remain",      (bool)t.m_bBlipRemain},
            {"is_interior", isInterior}
        };

        if (isInterior) {
            entry["interior_x"] = interiorPos.x;
            entry["interior_y"] = interiorPos.y;
            entry["interior_z"] = interiorPos.z;
        }

        arr.push_back(std::move(entry));
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
        g_fields["area_code"]   = readAreaCode;
        g_fields["is_interior"] = readIsInterior;

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
