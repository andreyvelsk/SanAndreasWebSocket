# SanAndreasWebSocket — Available Fields

Fields are used in `query` and `subscribe` methods.  
All values are read on the **game thread** — if the player entity is unavailable
(`FindPlayerPed()` returns `null`), the field value is `null`.

---

## Position & Orientation

| Field | JSON type | Description |
|-------|-----------|-------------|
| `position` | `{"x": float, "y": float, "z": float}` | World coordinates (composite) |
| `position_x` | `float` | X coordinate |
| `position_y` | `float` | Y coordinate |
| `position_z` | `float` | Z coordinate |
| `angle` | `float` | Heading in degrees, `0`–`360` (clockwise from north). Derived from `m_fHeadingCurrent` in radians. |

> `position` is the recommended field for tracking player movement — it returns
> all three axes in one value and is compared as a unit for change detection.

> In interiors `position` returns **world-space coordinates** (interior cells are placed
> at fixed locations outside the main map area). Use `area_code` to determine whether
> the player is in the open world (`0`) or in an interior.

---

## Vitals

| Field | JSON type | Range | Description |
|-------|-----------|-------|-------------|
| `health` | `float` | `0` … `200` | Player health (200 max with upgrades) |
| `armour` | `float` | `0` … `100` | Player armour |

---

## Economy & Law

| Field | JSON type | Description |
|-------|-----------|-------------|
| `money` | `integer` | Cash on hand (`CWorld::Players[0].m_nMoney`) |
| `wanted` | `integer` | Wanted level in **stars** (0–6, derived from raw level ÷ 100) |
| `wanted_raw` | `integer` | Raw wanted level value (0–9200+) |

---

## Ped State

| Field | JSON type | Description |
|-------|-----------|-------------|
| `ped_state` | `integer` | Raw `ePedState` enum value (see table below) |
| `move_state` | `integer` | Raw `eMoveState` enum value (see table below) |
| `in_vehicle` | `boolean` | `true` if player is seated in a vehicle |
| `area_code` | `integer` | Interior/area code: `0` = open world |

### ePedState values (ped_state)

| Value | Name | Description |
|-------|------|-------------|
| `0` | `PEDSTATE_NONE` | — |
| `1` | `PEDSTATE_IDLE` | Standing still |
| `2` | `PEDSTATE_IDLE_TAXI` | Waiting for taxi |
| `5` | `PEDSTATE_WANDER_RANGE` | Wandering |
| `6` | `PEDSTATE_WANDER_PATH` | Following path |
| `9` | `PEDSTATE_SEEK_CAR` | Looking for a car |
| `10` | `PEDSTATE_SEEK_CAR_ANY_MEANS` | Carjacking mode |
| `11` | `PEDSTATE_STUCK` | Stuck |
| `15` | `PEDSTATE_ENTER_CAR` | Getting into a vehicle |
| `16` | `PEDSTATE_STEAL_CAR` | Stealing a car |
| `17` | `PEDSTATE_EXIT_CAR` | Exiting a vehicle |
| `19` | `PEDSTATE_DRIVING` | Driving |
| `20` | `PEDSTATE_PASSENGER` | Passenger in vehicle |
| `21` | `PEDSTATE_TAXI_PASSENGER` | Taxi passenger |
| `25` | `PEDSTATE_ATTACK` | Attacking (shooting) |
| `26` | `PEDSTATE_FIGHT` | Fist fight |
| `29` | `PEDSTATE_AIMGUN` | Aiming gun |
| `31` | `PEDSTATE_SNIPER_MODE` | Sniper mode |
| `32` | `PEDSTATE_ROCKETLAUNCHER_MODE` | RPG mode |
| `51` | `PEDSTATE_JUMP` | Jumping |
| `53` | `PEDSTATE_FALL` | Falling |
| `54` | `PEDSTATE_ON_FIRE` | On fire |
| `55` | `PEDSTATE_DIE` | Dying |
| `56` | `PEDSTATE_DEAD` | Dead |
| `60` | `PEDSTATE_ARRESTED` | Being arrested |
| `75` | `PEDSTATE_MAKE_PHONECALL` | Making a phone call |
| `76` | `PEDSTATE_ANSWER_MOBILE` | Answering a call |

### eMoveState values (move_state)

| Value | Name | Description |
|-------|------|-------------|
| `0` | `PEDMOVE_NONE` | No movement (e.g., in vehicle) |
| `1` | `PEDMOVE_STILL` | Standing still |
| `2` | `PEDMOVE_TURN_L` | Turning left |
| `3` | `PEDMOVE_TURN_R` | Turning right |
| `4` | `PEDMOVE_WALK` | Walking |
| `5` | `PEDMOVE_JOG` | Jogging |
| `6` | `PEDMOVE_RUN` | Running |
| `7` | `PEDMOVE_SPRINT` | Sprinting |

---

## Weapon

| Field | JSON type | Description |
|-------|-----------|-------------|
| `current_weapon` | `integer` | `eWeaponType` of the equipped weapon |
| `weapon_slot` | `integer` | Current weapon slot (0–12) |
| `ammo_clip` | `integer` | Rounds in the current magazine |
| `ammo_total` | `integer` | Total rounds carried for current weapon |

### Common eWeaponType values (current_weapon)

| Value | Weapon |
|-------|--------|
| `0` | Unarmed / Fist |
| `1` | Brass Knuckles |
| `2` | Golf Club |
| `3` | Nightstick |
| `4` | Knife |
| `5` | Baseball Bat |
| `6` | Shovel |
| `7` | Pool Cue |
| `8` | Katana |
| `9` | Chainsaw |
| `10` | Purple Dildo |
| `22` | Pistol |
| `23` | Silenced Pistol |
| `24` | Desert Eagle |
| `25` | Shotgun |
| `26` | Sawn-off Shotgun |
| `27` | Combat Shotgun |
| `28` | Micro SMG / Uzi |
| `29` | SMG |
| `30` | AK-47 |
| `31` | M4 |
| `32` | Tec-9 |
| `33` | Country Rifle |
| `34` | Sniper Rifle |
| `35` | Rocket Launcher |
| `36` | Heat-Seeking RPG |
| `37` | Flamethrower |
| `38` | Minigun |
| `39` | Satchel Charge |
| `40` | Detonator |
| `41` | Spray Can |
| `42` | Fire Extinguisher |
| `43` | Camera |
| `44` | Night Vision |
| `45` | Thermal Vision |
| `46` | Parachute |

---

## Game Time

| Field | JSON type | Description |
|-------|-----------|-------------|
| `game_time` | `{"hour": int, "minute": int}` | In-game clock (composite) |
| `game_hour` | `integer` | In-game hour (0–23) |
| `game_minute` | `integer` | In-game minute (0–59) |

---

## Map Blips

| Field | JSON type | Description |
|-------|-----------|-------------|
| `blips` | `array` | All active radar blips. See [blips.md](blips.md) for the full reference |

The `blips` field returns an array of objects. Each object represents one active slot in `CRadar::ms_RadarTrace[175]`.

```json
[
  {
    "idx":         0,
    "type":        4,
    "sprite":      38,
    "display":     3,
    "color":       8,
    "x":           2496.0,
    "y":          -1667.6,
    "z":           13.4,
    "size":        2,
    "short_range": false,
    "friendly":    false
  }
]
```

| Sub-field | JSON type | Description |
|-----------|-----------|-------------|
| `idx` | `integer` | Slot index (0–174). Stable within a game session |
| `type` | `integer` | `eBlipType`: what entity the blip is attached to (`4` = fixed coordinate, `2` = character, `1` = car, …) |
| `sprite` | `integer` | `eRadarSprite`: icon identifier (`0` = coloured dot, `6` = Ammu-Nation, `38` = Sweet, …) |
| `display` | `integer` | `eBlipDisplay`: `0`=hidden, `1`=marker only, `2`=blip only, `3`=both |
| `color` | `integer` | `eBlipColour`: dot colour. Relevant when `sprite == 0` |
| `x`, `y`, `z` | `float` | World position. Updated every frame for entity blips (`type` 1/2/3/7) |
| `size` | `integer` | Relative dot size (`1` = smallest) |
| `short_range` | `boolean` | `true` → only visible on mini-map when nearby; `false` → always shown |
| `friendly` | `boolean` | Affects `BLIP_COLOUR_THREAT` colour selection |

> For the complete enum tables (`eBlipType`, `eRadarSprite`, `eBlipColour`, `eBlipDisplay`) and client-side filtering examples, see [blips.md](blips.md).

---

## Notes

- All `float` values use standard IEEE 754 double precision as JSON numbers.
- Composite fields (`position`, `game_time`) are compared as a unit for
  change detection in subscriptions — if any component changes, the whole
  object is pushed.
- The `blips` array is compared as a whole for change detection — any added,
  removed, or moved blip triggers a push.
- New fields can be added by registering them in `FieldRegistry::init()`
  in `src/protocol/FieldRegistry.cpp`.
