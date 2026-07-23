# RmlUi Custom Data Model Bindings

This document describes the custom RmlUi data model bindings provided by the WoTLK GUI Vulkan Layer. These bindings allow the game's UI overlay to display live data from the World of Warcraft (WotLK 3.3.5a) process.

## The `wow` Data Model

The layer exposes a single unified data model named **`wow`**. To use these bindings in your RmlUi layouts, your RML document's `<body>` element (or any container element wrapping the bound controls) must declare the `data-model` attribute:

```html
<body data-model="wow">
    <!-- UI elements with data bindings go here -->
</body>
```

---

## Bound Variables

The following variables are bound from C++ (`WowDataModel`) to RmlUi and updated once per frame.

### 1. Identity & Character Information

| Variable Name | RmlUi Type | C++ Mirror Variable | Description |
| :--- | :--- | :--- | :--- |
| `playerName` | `String` | `m_playerName` | The character's name. |
| `realmName` | `String` | `m_realmName` | The name of the game server/realm. |
| `playerHealth` | `int` | `m_playerHealth` | The current health of the player character. |
| `playerIsIngame` | `int` (0 or 1) | `m_playerIsIngame` | `1` if the player character is currently logged in and active in the world, `0` otherwise. |

### 2. World & Zone Location

| Variable Name | RmlUi Type | C++ Mirror Variable | Description |
| :--- | :--- | :--- | :--- |
| `continentName` | `String` | `m_continentName` | The name of the active continent. |
| `zoneText` | `String` | `m_zoneText` | The name of the main zone (e.g., "Dalarans", "Icecrown"). |
| `subZoneText` | `String` | `m_subZoneText` | The specific subzone name (e.g., "Krasus' Landing"). |
| `mapID` | `int` | `m_mapID` | The numerical ID of the active continent or map. |
| `zoneID` | `int` | `m_zoneID` | The numerical ID of the active zone. |

### 3. Game & Transition States

| Variable Name | RmlUi Type | C++ Mirror Variable | Description |
| :--- | :--- | :--- | :--- |
| `gameState` | `int` | `m_gameState` | The current game connection/UI state. |
| `worldLoaded` | `int` (0 or 1) | `m_worldLoaded` | `1` if the world map and surrounding objects are fully loaded, `0` otherwise. |
| `isLoading` | `int` (0 or 1) | `m_isLoading` | `1` if the client is currently showing a loading screen, `0` otherwise. |
| `isIndoor` | `int` (0 or 1) | `m_isIndoor` | `1` if the player is currently inside a building or cave, `0` otherwise. |
| `tickCount` | `int` | `m_tickCount` | Frame or update cycle tick count, incremented each time data is refreshed. |

### 4. Corpse Location

These coordinates represent where the player's corpse is located (useful for corpse runs).

| Variable Name | RmlUi Type | C++ Mirror Variable | Description |
| :--- | :--- | :--- | :--- |
| `corpseX` | `float` | `m_corpseX` | X-coordinate of the player's corpse in the world coordinate system. |
| `corpseY` | `float` | `m_corpseY` | Y-coordinate of the player's corpse. |
| `corpseZ` | `float` | `m_corpseZ` | Z-coordinate (altitude) of the player's corpse. |

### 5. Party, Raid & Quest Metrics

| Variable Name | RmlUi Type | C++ Mirror Variable | Description |
| :--- | :--- | :--- | :--- |
| `numPartyMembers` | `int` | `m_numPartyMembers` | The number of players currently in your party. |
| `partyDifficulty` | `int` | `m_partyDifficulty` | The current dungeon difficulty of your party. |
| `numRaidMembers` | `int` | `m_numRaidMembers` | The number of players currently in your raid. |
| `raidDifficulty` | `int` | `m_raidDifficulty` | The current raid difficulty of your raid. |
| `activeQuestsCount` | `int` | `m_activeQuestsCount` | The total number of active quests in your quest log. |

### 6. Lists & Arrays (Complex Bindings)

| Variable Name | RmlUi Type | C++ Mirror Variable | Description |
| :--- | :--- | :--- | :--- |
| `playerAuras` | `Vector<int>` | `m_playerAuras` | Array of Spell IDs representing the active auras (buffs/debuffs) on the player. |
| `targetAuras` | `Vector<int>` | `m_targetAuras` | Array of Spell IDs representing the active auras on the player's current target. |
| `combatLog` | `Vector<String>` | `m_combatLogHistory` | A list of formatted combat log events captured since loading. |
| `partyMembers` | `Vector<GroupMember>` | `m_partyMembers` | Array of party members, each with fields: `guid` (String), `name` (String), `health` (int), `maxHealth` (int), and `auras` (Vector<int>). |
| `raidMembers` | `Vector<GroupMember>` | `m_raidMembers` | Array of raid members, each with fields: `guid` (String), `name` (String), `health` (int), `maxHealth` (int), and `auras` (Vector<int>). |

---

## Usage Examples in RML

### 1. Direct Text Binding (Double Curly Braces)

In RmlUi, dynamic values are rendered directly into the document using double curly braces `{{ variable }}`.

```html
<div class="character-info">
    <h2>Character Status</h2>
    <p>Name: <strong>{{ playerName }}</strong></p>
    <p>Realm: {{ realmName }}</p>
    <p>Location: {{ zoneText }} ({{ subZoneText }})</p>
    <p>Health: <span class="hp-text">{{ playerHealth }}</span></p>
</div>
```

*Note: For text element content, do not use `data-value="..."`. Use double curly braces `{{ ... }}` instead, as `data-value` is reserved for two-way binding on interactive controls like text inputs, checkboxes, and select menus.*

### 2. Conditional Elements (`data-show` / `data-hide`)

You can dynamically show or hide elements based on game states using the `data-show` or `data-hide` attributes:

```html
<!-- Displayed only when the player is inside an indoor area -->
<div class="indoor-warning" data-show="isIndoor">
    <span>You are currently indoors. Mounts cannot be used.</span>
</div>

<!-- Displayed only when the player is dead / loading is not active -->
<div class="status-overlay" data-hide="playerIsIngame">
    <span>Please log in to display character stats...</span>
</div>
```

### 3. Iterating Arrays (`data-for`)

Use the `data-for` attribute to repeat an element for each item in an array (like player auras or combat logs).

#### Iterating Player Aura Spell IDs:
```html
<h2>Active Buffs</h2>
<div class="auras-list">
    <!-- This span will be repeated for every active aura, printing the spell ID -->
    <span class="aura-icon" data-for="spellId : playerAuras">
        Spell ID: {{ spellId }}
    </span>
</div>
```

#### Iterating Combat Log History:
```html
<h2>Recent Combat Events</h2>
<div class="combat-log">
    <div class="log-row" data-for="event : combatLog">
        {{ event }}
    </div>
</div>
```

#### Iterating Party Members (with health and auras):
```html
<div class="party-list">
    <div class="member-frame" data-for="member : partyMembers">
        <span class="name">{{ member.name }}</span>
        <span class="hp">{{ member.health }} / {{ member.maxHealth }}</span>

        <!-- Member Buffs -->
        <div class="member-buffs">
            <span class="buff" data-for="spellId : member.auras">
                {{ spellId }}
            </span>
        </div>
    </div>
</div>
```

---

## Styling with RCSS

RmlUi stylesheets (.rcss) can style the elements bound to the data model. Be sure to respect RmlUi restrictions:
- Split declarations (e.g., use `border-width: 2px; border-color: #ff0000; border-style: solid;` instead of the composite shorthand `border: 2px solid #ff0000;`).
- Inside RCSS, specify single font families (such as `font-family: DejaVuSans;`) instead of comma-separated fallback lists.

Example snippet (`overlay.rcss`):
```css
body {
    font-family: DejaVuSans;
    font-size: 14px;
    color: #ffffff;
}

#overlay {
    width: 350px;
    background-color: rgba(20, 20, 20, 0.85);
    border-width: 2px;
    border-style: solid;
    border-color: #444444;
    padding: 15px;
    border-radius: 8px;
}

.auras-list {
    display: flex;
    flex-wrap: wrap;
    gap: 5px;
}

.aura-icon {
    background-color: #2a2a2a;
    border-width: 1px;
    border-style: solid;
    border-color: #00ff66;
    padding: 3px 6px;
    border-radius: 4px;
    font-size: 11px;
}
```
