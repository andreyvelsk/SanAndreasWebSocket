# SanAndreasWebSocket — Map Blips (Radar Markers)

> **Data source:** `CRadar::ms_RadarTrace[175]` — array of up to 175 slots  
> **Struct:** `tRadarTrace` (`game_sa/CRadar.h`)  
> **API field:** `blips` — available via standard `query` and `subscribe` methods

---

## Table of Contents

1. [How the blip system works](#1-how-the-blip-system-works)
2. [tRadarTrace struct](#2-tradartrace-struct)
3. [Blip categories](#3-blip-categories)
4. [Reference: eBlipType](#4-ebliptype)
5. [Reference: eRadarSprite (icons)](#5-eradarsprite)
6. [Reference: eBlipColour](#6-eblipcolour)
7. [Reference: eBlipDisplay](#7-eblipdisplay)
8. [3D markers (C3dMarkers)](#8-3d-markers)
9. [API — field blips](#9-api--field-blips)
10. [Examples](#10-examples)

---

## 1. How the blip system works

All radar markers in GTA San Andreas — on the mini-map and the full-screen map — are stored in a **single** array: `CRadar::ms_RadarTrace[175]`. There is no separate system for different marker types; it is one unified array.

Each slot is either free (`m_bInUse == false`) or holds an active blip. The game does not split blips into "static", "mission", or "dynamic" categories. All blips share the `tRadarTrace` structure; what differs is:

| Field | Purpose |
|-------|---------|
| `m_nBlipType` (`eBlipType`) | What the blip is attached to: an entity (vehicle/character) or fixed coordinates |
| `m_nRadarSprite` (`eRadarSprite`) | Which icon to display. This is the **primary identifier** for rendering on the map |
| `m_bShortRange` | Whether the blip is always visible or only appears when the player is nearby |

**Entity blips** (`BLIP_CAR`, `BLIP_CHAR`, `BLIP_OBJECT`, `BLIP_PICKUP`) are "dynamic": their position follows a live game entity.  
**Coordinate blips** (`BLIP_COORD`, `BLIP_CONTACTPOINT`) are "static": the position is set by the script and does not change.

### Visibility in interiors

The game uses `CRadar::DisplayThisBlip(sprite, priority)` to decide whether a blip is rendered in the current context. When the player is **inside a building** (`CGame::CanSeeOutSideFromCurrArea() == false`), this function allows only a handful of sprites through: `RADAR_SPRITE_NONE`, `RADAR_SPRITE_WHITE`, `RADAR_SPRITE_MAFIACASINO`, `RADAR_SPRITE_SCHOOL`, `RADAR_SPRITE_WAYPOINT`, `RADAR_SPRITE_TRIADSCASINO`, `RADAR_SPRITE_CASH`. All other sprites — including all POI and contact icons — are suppressed.

In the **open world** all sprites pass (each sprite category is covered by one of the three draw priorities that `DrawBlips` iterates).

Additionally, `BLIP_CONTACTPOINT` blips (entrance spheres, objective areas) are suppressed by `CRadar::DrawCoordBlip` whenever `CTheScripts::IsPlayerOnAMission()` is true.

---

## 2. tRadarTrace struct

```cpp
struct tRadarTrace {
    unsigned int   m_nColour;         // dot colour (eBlipColour)
    unsigned int   m_nEntityHandle;   // entity handle (for entity blips)
    CVector        m_vecPos;          // world-space position
    unsigned short m_nCounter;        // generation counter (used in tBlipHandle)
    float          m_fSphereRadius;   // sphere radius (for BLIP_CONTACTPOINT)
    unsigned short m_nBlipSize;       // size (1 = smallest)
    CEntryExit    *m_pEntryExit;      // interior entry point (if any)
    unsigned char  m_nRadarSprite;    // icon (eRadarSprite)
    // bit fields:
    unsigned char  m_bBright          : 1; // bright colour (almost always true)
    unsigned char  m_bInUse           : 1; // slot is occupied
    unsigned char  m_bShortRange      : 1; // not shown on full-screen map
    unsigned char  m_bFriendly        : 1; // affects BLIP_COLOUR_THREAT colour
    unsigned char  m_bBlipRemain      : 1; // blip persists after entity deletion
    unsigned char  m_bBlipFade        : 1; // unused
    unsigned char  m_nCoordBlipAppearance : 2; // eBlipAppearance (FRIEND/THREAT)
    unsigned char  m_nBlipDisplay     : 2; // eBlipDisplay
    unsigned char  m_nBlipType        : 4; // eBlipType
};
```

> **Entity blip position:** for `BLIP_CAR`, `BLIP_CHAR`, `BLIP_OBJECT`, the game reads the position **directly from the live entity** via `GetPosition()` during `DrawEntityBlip`. `m_vecPos` in `tRadarTrace` is **not** updated by the game for entity-tracking blips — it is only used as a fallback when the entity is gone but `m_bBlipRemain` keeps the blip alive. For coordinate blips (`BLIP_COORD`, `BLIP_CONTACTPOINT`) and `BLIP_PICKUP`, `m_vecPos` is set by the script and used as-is.

---

## 3. Blip categories

Despite the unified struct, blips can be grouped into three practical categories based on `m_nBlipType` + `m_nRadarSprite`:

### 3.1 Permanent POI (Static)

**Coordinate blips** (`m_nBlipType == BLIP_COORD`) with location icons:

| Icons | Description |
|-------|-------------|
| `RADAR_SPRITE_AMMUGUN` (6) | Ammu-Nation |
| `RADAR_SPRITE_BARBERS` (7) | Barber shop |
| `RADAR_SPRITE_BURGERSHOT` (10) | Burger Shot |
| `RADAR_SPRITE_CHICKEN` (14) | Cluckin' Bell |
| `RADAR_SPRITE_DINER` (17) | Diner |
| `RADAR_SPRITE_PIZZA` (29) | Pizza restaurant |
| `RADAR_SPRITE_HOSTPITAL` (22) | Hospital |
| `RADAR_SPRITE_POLICE` (30) | Police station |
| `RADAR_SPRITE_SAVEGAME` (35) | Save point |
| `RADAR_SPRITE_MODGARAGE` (27) | Mod garage |
| `RADAR_SPRITE_GYM` (54) | Gym |
| `RADAR_SPRITE_TATTOO` (39) | Tattoo parlour |
| `RADAR_SPRITE_SCHOOL` (36) | Driving / pilot school |
| `RADAR_SPRITE_BOATYARD` (9) | Boatyard |
| `RADAR_SPRITE_AIRYARD` (5) | Airfield |
| `RADAR_SPRITE_IMPOUND` (55) | Impound lot |
| `RADAR_SPRITE_SPRAY` (63) | Pay'n'Spray |
| `RADAR_SPRITE_PROPERTYG` (31) | Owned property (green) |
| `RADAR_SPRITE_PROPERTYR` (32) | Property for sale (red) |
| `RADAR_SPRITE_RACE` (33) | Race checkpoint |
| `RADAR_SPRITE_RUNWAY` (57) | Runway |
| `RADAR_SPRITE_LIGHT` (56) | Spotlight |

### 3.2 Mission / Scripted

Added and removed by the SCM script. Type is `BLIP_COORD` or `BLIP_CHAR`:

| Icons | Description |
|-------|-------------|
| `RADAR_SPRITE_BIGSMOKE` (8) | Contact: Big Smoke |
| `RADAR_SPRITE_CATALINAPINK` (12) | Contact: Catalina |
| `RADAR_SPRITE_CESARVIAPANDO` (13) | Contact: Cesar Vialpando |
| `RADAR_SPRITE_CRASH1` (16) | Contact: C.R.A.S.H. (Tenpenny) |
| `RADAR_SPRITE_EMMETGUN` (18) | Contact: Emmet |
| `RADAR_SPRITE_LOGOSYNDICATE` (23) | Syndicate |
| `RADAR_SPRITE_MADDOG` (24) | Contact: Mad Dogg |
| `RADAR_SPRITE_MAFIACASINO` (25) | Mafia Casino |
| `RADAR_SPRITE_MCSTRAP` (26) | Contact: MC Strap |
| `RADAR_SPRITE_OGLOC` (28) | Contact: OG Loc |
| `RADAR_SPRITE_RYDER` (34) | Contact: Ryder |
| `RADAR_SPRITE_SWEET` (38) | Contact: Sweet |
| `RADAR_SPRITE_THETRUTH` (40) | Contact: The Truth |
| `RADAR_SPRITE_TORENORANCH` (42) | Contact: Toreno |
| `RADAR_SPRITE_TRIADS` (43) | Triads |
| `RADAR_SPRITE_TRIADSCASINO` (44) | Triads Casino |
| `RADAR_SPRITE_WOOZIE` (46) | Contact: Woozie |
| `RADAR_SPRITE_ZERO` (47) | Contact: Zero |
| `RADAR_SPRITE_QMARK` (37) | Question mark (unknown mission) |
| `RADAR_SPRITE_FLAG` (53) | Finish / flag |
| `RADAR_SPRITE_CASH` (52) | Cash objective |
| `RADAR_SPRITE_TRUCK` (51) | Truck objective |
| `RADAR_SPRITE_WAYPOINT` (41) | User waypoint |

### 3.3 Dynamic (Entity-tracking)

**Entity blips** — attached to live game objects; position updates every frame:

| `eBlipType` | Description |
|-------------|-------------|
| `BLIP_CHAR` (2) | Pedestrian / character. Icons: `RADAR_SPRITE_CJ` (15), `RADAR_SPRITE_GIRLFRIEND` (21), `RADAR_SPRITE_ENEMYATTACK` (19), `RADAR_SPRITE_NONE` (0 — coloured dot) |
| `BLIP_CAR` (1) | Vehicle. Usually `RADAR_SPRITE_NONE` (coloured dot) or `RADAR_SPRITE_TRUCK` (51) |
| `BLIP_OBJECT` (3) | Object (mission target item) |
| `BLIP_PICKUP` (7) | Pickup (item on the ground) |

Blips with `m_bShortRange == true` are visible only on the mini-map (while the player is nearby) and do not appear on the full-screen map.

### 3.4 Special

| `eBlipType` | Description |
|-------------|-------------|
| `BLIP_SPOTLIGHT` (6) | Spotlight (helicopter search beam, etc.) |
| `BLIP_AIRSTRIP` (8) | Airstrip (airfield landing zone) |
| `BLIP_CONTACTPOINT` (5) | Sphere — a 3D target area (interior entry, objective radius) |

---

## 4. eBlipType

```cpp
enum eBlipType {
    BLIP_NONE         = 0,  // slot is free / inactive
    BLIP_CAR          = 1,  // vehicle
    BLIP_CHAR         = 2,  // pedestrian / character
    BLIP_OBJECT       = 3,  // object
    BLIP_COORD        = 4,  // fixed coordinates (Checkpoint)
    BLIP_CONTACTPOINT = 5,  // sphere at fixed coordinates
    BLIP_SPOTLIGHT    = 6,  // spotlight
    BLIP_PICKUP       = 7,  // pickup
    BLIP_AIRSTRIP     = 8   // airstrip
};
```

**Determining blip nature:**

| `m_nBlipType` | Nature | Position |
|---------------|--------|---------|
| 1, 2, 3, 7 | Dynamic (entity) | Resolved from the entity via `GetPosition()` each read; `m_vecPos` is a fallback when entity is gone but `m_bBlipRemain` is true |
| 4, 5, 6, 8 | Static (coordinate) | `m_vecPos` is set by the script and does not change |

---

## 5. eRadarSprite

The value of `m_nRadarSprite` is the **primary icon identifier** for rendering on the map. `0` means "coloured dot" (no icon sprite; the dot colour comes from `m_nColour`).

| Value | Constant | Visual icon | Category |
|-------|----------|-------------|----------|
| `0` | `RADAR_SPRITE_NONE` | Coloured dot (colour from `m_nColour`) | — |
| `1` | `RADAR_SPRITE_WHITE` | White dot | — |
| `2` | `RADAR_SPRITE_CENTRE` | Map centre | System |
| `3` | `RADAR_SPRITE_MAP_HERE` | "You are here" | System |
| `4` | `RADAR_SPRITE_NORTH` | North arrow | System |
| `5` | `RADAR_SPRITE_AIRYARD` | Airfield / plane | POI |
| `6` | `RADAR_SPRITE_AMMUGUN` | Ammu-Nation | POI |
| `7` | `RADAR_SPRITE_BARBERS` | Barber shop | POI |
| `8` | `RADAR_SPRITE_BIGSMOKE` | Big Smoke | Mission |
| `9` | `RADAR_SPRITE_BOATYARD` | Boatyard | POI |
| `10` | `RADAR_SPRITE_BURGERSHOT` | Burger Shot | POI |
| `11` | `RADAR_SPRITE_BULLDOZER` | Bulldozer | Mission |
| `12` | `RADAR_SPRITE_CATALINAPINK` | Catalina | Mission |
| `13` | `RADAR_SPRITE_CESARVIAPANDO` | Cesar Vialpando | Mission |
| `14` | `RADAR_SPRITE_CHICKEN` | Cluckin' Bell | POI |
| `15` | `RADAR_SPRITE_CJ` | CJ (player) | Dynamic |
| `16` | `RADAR_SPRITE_CRASH1` | C.R.A.S.H. / Tenpenny | Mission |
| `17` | `RADAR_SPRITE_DINER` | Diner | POI |
| `18` | `RADAR_SPRITE_EMMETGUN` | Emmet | Mission |
| `19` | `RADAR_SPRITE_ENEMYATTACK` | Enemy / attack target | Dynamic |
| `20` | `RADAR_SPRITE_FIRE` | Fire | Mission |
| `21` | `RADAR_SPRITE_GIRLFRIEND` | Girlfriend | POI / Mission |
| `22` | `RADAR_SPRITE_HOSTPITAL` | Hospital | POI |
| `23` | `RADAR_SPRITE_LOGOSYNDICATE` | Syndicate | Mission |
| `24` | `RADAR_SPRITE_MADDOG` | Mad Dogg | Mission |
| `25` | `RADAR_SPRITE_MAFIACASINO` | Mafia Casino | Mission |
| `26` | `RADAR_SPRITE_MCSTRAP` | MC Strap | Mission |
| `27` | `RADAR_SPRITE_MODGARAGE` | Mod garage | POI |
| `28` | `RADAR_SPRITE_OGLOC` | OG Loc | Mission |
| `29` | `RADAR_SPRITE_PIZZA` | Pizza restaurant | POI |
| `30` | `RADAR_SPRITE_POLICE` | Police station | POI |
| `31` | `RADAR_SPRITE_PROPERTYG` | Owned property | POI |
| `32` | `RADAR_SPRITE_PROPERTYR` | Property for sale | POI |
| `33` | `RADAR_SPRITE_RACE` | Race | Mission |
| `34` | `RADAR_SPRITE_RYDER` | Ryder | Mission |
| `35` | `RADAR_SPRITE_SAVEGAME` | Save point | POI |
| `36` | `RADAR_SPRITE_SCHOOL` | Driving / pilot school | POI |
| `37` | `RADAR_SPRITE_QMARK` | Question mark (mission) | Mission |
| `38` | `RADAR_SPRITE_SWEET` | Sweet | Mission |
| `39` | `RADAR_SPRITE_TATTOO` | Tattoo parlour | POI |
| `40` | `RADAR_SPRITE_THETRUTH` | The Truth | Mission |
| `41` | `RADAR_SPRITE_WAYPOINT` | User waypoint | Navigation |
| `42` | `RADAR_SPRITE_TORENORANCH` | Toreno Ranch | Mission |
| `43` | `RADAR_SPRITE_TRIADS` | Triads | Mission |
| `44` | `RADAR_SPRITE_TRIADSCASINO` | Triads Casino | Mission |
| `45` | `RADAR_SPRITE_TSHIRT` | Clothing store | POI |
| `46` | `RADAR_SPRITE_WOOZIE` | Woozie | Mission |
| `47` | `RADAR_SPRITE_ZERO` | Zero (RC shop) | Mission / POI |
| `48` | `RADAR_SPRITE_DATEDISCO` | Date: nightclub | Dynamic |
| `49` | `RADAR_SPRITE_DATEDRINK` | Date: bar | Dynamic |
| `50` | `RADAR_SPRITE_DATEFOOD` | Date: restaurant | Dynamic |
| `51` | `RADAR_SPRITE_TRUCK` | Truck objective | Mission |
| `52` | `RADAR_SPRITE_CASH` | Cash / money objective | Mission |
| `53` | `RADAR_SPRITE_FLAG` | Finish / flag | Mission |
| `54` | `RADAR_SPRITE_GYM` | Gym | POI |
| `55` | `RADAR_SPRITE_IMPOUND` | Impound lot | POI |
| `56` | `RADAR_SPRITE_LIGHT` | Spotlight | Dynamic |
| `57` | `RADAR_SPRITE_RUNWAY` | Runway | POI |
| `58` | `RADAR_SPRITE_GANGB` | Gang (blue) | Mission |
| `59` | `RADAR_SPRITE_GANGP` | Gang (purple) | Mission |
| `60` | `RADAR_SPRITE_GANGY` | Gang (yellow) | Mission |
| `61` | `RADAR_SPRITE_GANGN` | Gang (neutral) | Mission |
| `62` | `RADAR_SPRITE_GANGG` | Grove Street gang (green) | Mission |
| `63` | `RADAR_SPRITE_SPRAY` | Pay'n'Spray | POI |

> Values `64` (`RADAR_SPRITE_TORENO`) and above are reserved; not used in the final game.

---

## 6. eBlipColour

Relevant only when `m_nRadarSprite == RADAR_SPRITE_NONE` (or `RADAR_SPRITE_WHITE`) — otherwise the icon sprite is drawn with its own colour.

| Value | Constant | Description |
|-------|----------|-------------|
| `0` | `BLIP_COLOUR_RED` | Red |
| `1` | `BLIP_COLOUR_GREEN` | Green |
| `2` | `BLIP_COLOUR_BLUE` | Blue |
| `3` | `BLIP_COLOUR_WHITE` | White |
| `4` | `BLIP_COLOUR_YELLOW` | Yellow |
| `5` | `BLIP_COLOUR_REDCOPY` | Red (duplicate) |
| `6` | `BLIP_COLOUR_BLUECOPY` | Blue (duplicate) |
| `7` | `BLIP_COLOUR_THREAT` | Threat: red if `m_bFriendly == false`, blue if `true` |
| `8` | `BLIP_COLOUR_DESTINATION` | Destination colour (default, usually yellow) |

---

## 7. eBlipDisplay

Controls where the marker is visible.

| Value | Constant | Description |
|-------|----------|-------------|
| `0` | `BLIP_DISPLAY_NEITHER` | Hidden (not displayed anywhere) |
| `1` | `BLIP_DISPLAY_MARKER_ONLY` | 3D ground marker only (no radar dot) |
| `2` | `BLIP_DISPLAY_BLIP_ONLY` | Radar dot only (no 3D ground marker) |
| `3` | `BLIP_DISPLAY_BOTH` | Both 3D marker and radar dot |

> Blips with `BLIP_DISPLAY_NEITHER` are technically active (`m_bInUse == true`) but invisible. The server returns them; the client may filter them out.

---

## 8. 3D Markers

In addition to radar blips, the game has **3D markers** (`C3dMarkers`) — physical shapes on the ground (cones, cylinders, arrows) that are visible in the world but **not** on the radar or mini-map.

Array: `C3dMarkers::m_aMarkerArray[32]` (up to 32 markers).

```cpp
enum e3dMarkerType {
    MARKER3D_ARROW             = 0,  // vertical arrow
    MARKER3D_CYLINDER          = 1,  // cylinder
    MARKER3D_TUBE              = 2,  // tube
    MARKER3D_ARROW2            = 3,  // arrow (variant 2)
    MARKER3D_TORUS             = 4,  // torus (ring)
    MARKER3D_CONE              = 5,  // cone
    MARKER3D_CONE_NO_COLLISION = 6,  // cone without collision
    MARKER3D_NA                = 257 // inactive
};
```

`C3dMarker` fields:

| Field | Type | Description |
|-------|------|-------------|
| `m_nType` | `e3dMarkerType` | Marker shape |
| `m_bIsUsed` | `bool` | Slot is occupied |
| `m_nIdentifier` | `int` | Unique ID (set by script) |
| `m_colour` | `CRGBA` | Colour (r, g, b, a) |
| `m_fSize` | `float` | Current radius / size |
| Position | via `m_mat` | `m_mat.GetPosition()` — world coordinates |

> 3D markers typically accompany coordinate radar blips (an arrow or ring on the ground under the icon). For a companion map, radar blips are more informative; 3D markers are supplementary data if needed.

---

## 9. API — field `blips`

The `blips` field uses the standard `query` and `subscribe` methods (see [protocol.md](protocol.md)).

### Single blip object format

```json
{
  "idx":         5,
  "type":        4,
  "sprite":      38,
  "display":     3,
  "color":       1,
  "x":           2496.0,
  "y":          -1667.6,
  "z":           13.4,
  "size":        2,
  "short_range": false,
  "friendly":    false,
  "remain":      true,
  "is_interior": false
}
```

**Interior blip** (save house, barbershop, restaurant, etc.) — `x,y,z` shows the **entrance door** position:
```json
{
  "idx":         12,
  "type":        4,
  "sprite":      35,
  "display":     3,
  "color":       8,
  "x":           2500.0,
  "y":          -1650.0,
  "z":           12.5,
  "size":        2,
  "short_range": false,
  "friendly":    false,
  "remain":      false,
  "is_interior": true,
  "interior_x":  3000.0,
  "interior_y": -2500.0,
  "interior_z":  950.0
}
```

| Field | JSON type | Source | Description |
|-------|-----------|--------|-------------|
| `idx` | `integer` | Index in `ms_RadarTrace[0..174]` | Stable blip ID within a game session |
| `type` | `integer` | `m_nBlipType` | Entity type (see [§4 eBlipType](#4-ebliptype)) |
| `sprite` | `integer` | `m_nRadarSprite` | Display icon (see [§5 eRadarSprite](#5-eradarsprite)). `0` = coloured dot |
| `display` | `integer` | `m_nBlipDisplay` | Visibility mode (see [§7 eBlipDisplay](#7-eblipdisplay)) |
| `color` | `integer` | `m_nColour` | Dot colour (see [§6 eBlipColour](#6-eblipcolour)). Relevant when `sprite == 0` |
| `x`, `y`, `z` | `float` | Entity `GetPosition()` for `BLIP_CAR/CHAR/OBJECT/PICKUP`; `m_vecPos` for coordinate blips and as fallback when entity is gone | World position. For entity blips, resolved from the live entity every read — reflects real-time movement. For coordinate blips, static value set by the script |
| `size` | `integer` | `m_nBlipSize` | Relative dot size (1 = smallest) |
| `short_range` | `boolean` | `m_bShortRange` | `true` — mini-map only (nearby); `false` — always shown |
| `friendly` | `boolean` | `m_bFriendly` | Affects `BLIP_COLOUR_THREAT` colour selection |
| `remain` | `boolean` | `m_bBlipRemain` | Актуален только для entity blips (`type` 1/2/3/7). `true` — blip останется в массиве даже после удаления персонажа/машины/объекта, за которым следил (позиция заморозится на последнем известном `m_vecPos`). `false` — blip будет автоматически очищен игрой при удалении entity. Для координатных blips (`type` 4/5/6/8) всегда `false`. |
| `is_interior` | `boolean` | `m_pEntryExit != nullptr` | `true` — blip связан с интерьером (дом сохранения, парикмахерская, ресторан и т.п.). `x,y,z` содержат позицию **входной двери** (центр `m_recEntrance` + `m_fEntranceZ`), соответствующую отображению на мини-карте. |
| `interior_x` | `float` | Исходный `m_vecPos` | **Только когда `is_interior == true`.** Исходная позиция blip в мире (координаты комнаты интерьера, находящиеся далеко за пределами основной карты). |
| `interior_y` | `float` | Исходный `m_vecPos` | См. `interior_x`. |
| `interior_z` | `float` | Исходный `m_vecPos` | См. `interior_x`. |

### Server-side processing

The server reads `CRadar::ms_RadarTrace` directly and applies the same filtering as `CRadar::DrawBlips()` to ensure the list matches exactly what the game draws on the minimap. Processing steps, in order:

1. **Slot inactive** — `m_bInUse == false` or `m_nBlipType == BLIP_NONE` → excluded.
2. **System sprite** — sprite is `RADAR_SPRITE_CENTRE` (2), `RADAR_SPRITE_MAP_HERE` (3), or `RADAR_SPRITE_NORTH` (4) → excluded. These are rendered outside the main blip loop by the game.
3. **`CRadar::DisplayThisBlip` check** — the server calls `CRadar::DisplayThisBlip(sprite, priority)` for `priority = 1, 2, 3` (replicating the priority loop in `DrawBlips`). A blip passes if the function returns `true` for at least one priority. The key area-dependent behaviour:
   - **Open world** (`CGame::CanSeeOutSideFromCurrArea() == true` and `area_code == 0`): every non-system sprite passes (POI sprites at priority 1, contacts/mission sprites at priority 3, others at priority 2). No blips are suppressed in the open world.
   - **Interior** (`CGame::CanSeeOutSideFromCurrArea() == false`): only a small fixed set of sprites is allowed — `RADAR_SPRITE_NONE`, `RADAR_SPRITE_WHITE`, `RADAR_SPRITE_MAFIACASINO`, `RADAR_SPRITE_SCHOOL`, `RADAR_SPRITE_WAYPOINT`, `RADAR_SPRITE_TRIADSCASINO`, `RADAR_SPRITE_CASH`. All other sprites (including POI and contact blips) are excluded. This matches what the minimap shows when the player is inside a building.
4. **Contact-point blips during missions** — if `m_nBlipType == BLIP_CONTACTPOINT` (5) **and** `CTheScripts::IsPlayerOnAMission() == true` → excluded. This matches the guard in `CRadar::DrawCoordBlip` that suppresses entrance spheres and objective areas while a mission is active.
5. **Entity position resolution** — for `BLIP_CAR`, `BLIP_CHAR`, `BLIP_OBJECT`, `BLIP_PICKUP`, the server reads the position from the live entity via `GetPosition()` (matching the game's `DrawEntityBlip` behaviour). If the entity no longer exists:
   - `m_bBlipRemain == true` → blip is included with the last known position from `m_vecPos`
   - `m_bBlipRemain == false` → excluded (the game clears these blips during the draw pass)
6. **Explicitly hidden** — `m_nBlipDisplay == BLIP_DISPLAY_NEITHER` (0) → excluded. The game does not draw these at all.
7. **Interior entrance resolution** — if `m_pEntryExit != nullptr`, the blip is an interior marker. The server computes the entrance door position (centre of `m_recEntrance` rect + `m_fEntranceZ`) and uses it for `x,y,z`. The original interior room coordinates are preserved in `interior_x/y/z`, and `is_interior` is set to `true`. This matches the game's `DrawCoordBlip` behaviour.

Blips that pass all checks are included. The client can use `display` to further filter:
- `display == 1` (`BLIP_DISPLAY_MARKER_ONLY`): 3D marker in the world, no radar dot
- `display == 2` (`BLIP_DISPLAY_BLIP_ONLY`): radar dot only, no 3D marker
- `display == 3` (`BLIP_DISPLAY_BOTH`): both radar dot and 3D marker

---

## 10. Examples

### Get all active blips (one-shot)

```json
→ {"jsonrpc":"2.0","method":"query","params":{"fields":["blips"]},"id":"b1"}
← {
    "jsonrpc": "2.0",
    "result": {
      "ts": 3600000,
      "fields": {
        "blips": [
          {"idx":0,"type":4,"sprite":38,"display":3,"color":8,"x":2496.0,"y":-1667.6,"z":13.4,"size":2,"short_range":false,"friendly":false,"remain":false},
          {"idx":1,"type":4,"sprite":6, "display":3,"color":8,"x":255.0, "y":-180.0, "z":1.5, "size":2,"short_range":false,"friendly":false,"remain":false},
          {"idx":2,"type":2,"sprite":0, "display":2,"color":0,"x":1234.5,"y":-900.0, "z":15.0,"size":1,"short_range":true, "friendly":false,"remain":true}
        ]
      }
    },
    "id": "b1"
  }
```

### Subscribe to blip changes every 2 seconds

```json
→ {"jsonrpc":"2.0","method":"subscribe","params":{"fields":["blips"],"interval":2000},"id":"s1"}
← {"jsonrpc":"2.0","result":{"subscribed":["blips"],"interval":2000},"id":"s1"}

// whenever a blip is added, removed, or moves:
← {
    "jsonrpc":"2.0","method":"data",
    "params":{"ts":3602000,"fields":{"blips":[...]}}
  }
```

### Get player position and all blips at once

```json
→ {"jsonrpc":"2.0","method":"query","params":{"fields":["position","blips"]},"id":"map1"}
```

### Client-side filtering examples

```js
// Permanent POI only (static coordinate blips, always shown)
const poi = blips.filter(b => b.type === 4 && b.sprite > 4 && !b.short_range);

// Mission targets (coloured dots, always shown)
const targets = blips.filter(b => b.type === 4 && b.sprite === 0 && !b.short_range);

// Dynamic entity blips only (skipping ghosts with invalid entities)
const entities = blips.filter(b => [1, 2, 3, 7].includes(b.type) && b.remain);

// Visible blips on minimap (exclude 3D-marker-only and hidden ones)
const minimap = blips.filter(b => b.display === 2 || b.display === 3);
```
