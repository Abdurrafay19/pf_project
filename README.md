# Space Shooter Game - Complete main.cpp Documentation

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Includes & Namespaces](#includes--namespaces)
3. [Global Constants](#global-constants)
4. [The main() Function Deep Dive](#the-main-function-deep-dive)
5. [Core Data Structures](#core-data-structures)
6. [Asset Loading System](#asset-loading-system)
7. [Game State Machine](#game-state-machine)
8. [Timing System](#timing-system)
9. [The Game Loop](#the-game-loop)
10. [State-Specific Logic](#state-specific-logic)
11. [Entity Movement & Collision](#entity-movement--collision)
12. [Rendering Pipeline](#rendering-pipeline)
13. [How Everything Links Together](#how-everything-links-together)

---

## 1. Architecture Overview

The game uses a **monolithic procedural architecture** where all code resides in a single `main()` function. This design pattern is common for small arcade-style games and makes the flow explicit and easy to follow.

**Key Design Principles:**
- **State Machine Pattern**: The game switches between different states (Menu, Playing, Paused, etc.)
- **Grid-Based World**: All entities exist in a 2D integer array representing the game world
- **Frame-Based Updates**: Logic executes every frame (60 FPS)
- **Immediate Mode Rendering**: The entire screen is redrawn each frame based on current state

---

## 2. Includes & Namespaces

```cpp
#include <SFML/Graphics.hpp>  // Core SFML graphics library
#include <iostream>            // For console output (debugging)
#include <cstdlib>             // For rand() and srand()
#include <ctime>               // For time() to seed random generator
using namespace std;           // Standard library namespace
using namespace sf;            // SFML namespace
```

**Why these includes:**
- `SFML/Graphics.hpp`: Provides window management, rendering, textures, sprites, text, and input handling
- `iostream`: Used for `cout` statements to debug game events in the console
- `cstdlib` & `ctime`: Random number generation for spawning entities at unpredictable positions

---

## 3. Global Constants

### Grid Configuration
```cpp
const int ROWS = 23;        // Vertical grid cells
const int COLS = 15;        // Horizontal grid cells
const int CELL_SIZE = 40;   // Each cell is 40x40 pixels
const int MARGIN = 40;      // Border around the game grid
```

**How they connect:**
- Window size is calculated as: `COLS * CELL_SIZE + MARGIN * 2 + 500` (width), `ROWS * CELL_SIZE + MARGIN * 2` (height)
- Every entity position is mapped: `pixelX = MARGIN + col * CELL_SIZE`
- The grid acts as a coordinate system: `grid[row][col]`

### Game States
```cpp
const int STATE_MENU = 0;
const int STATE_PLAYING = 1;
const int STATE_INSTRUCTIONS = 2;
const int STATE_GAME_OVER = 3;
const int STATE_LEVEL_UP = 4;
const int STATE_VICTORY = 5;
const int STATE_PAUSED = 6;
```

**State Machine Flow:**
```
STATE_MENU ←→ STATE_INSTRUCTIONS
    ↓
STATE_PLAYING ←→ STATE_PAUSED
    ↓
STATE_LEVEL_UP → (loops back to STATE_PLAYING)
    ↓
STATE_GAME_OVER or STATE_VICTORY → back to STATE_MENU
```

---

## 4. The main() Function Deep Dive

### Phase 1: Initialization (Lines 19-21)
```cpp
srand(static_cast<unsigned int>(time(0)));
```
**Purpose**: Seeds the random number generator with the current timestamp
**Impact**: Ensures meteor/enemy spawn positions are different each game session

### Phase 2: Window Creation (Lines 23-26)
```cpp
const int windowWidth = COLS * CELL_SIZE + MARGIN * 2 + 500;
const int windowHeight = ROWS * CELL_SIZE + MARGIN * 2;
RenderWindow window(VideoMode(windowWidth, windowHeight), "Space Shooter");
window.setFramerateLimit(60);
```

**Breakdown:**
- `windowWidth`: Grid area (600px) + margins (80px) + UI panel (500px) = 1180px
- `windowHeight`: Grid area (920px) + margins (80px) = 1000px
- `setFramerateLimit(60)`: Caps execution to 60 iterations/second for consistent gameplay

---

## 5. Core Data Structures

### The Grid System (Lines 49-51)
```cpp
int grid[ROWS][COLS] = {0};
```

**This is the heart of the game.** Every entity exists as an integer value in this 2D array:

| Value | Entity Type | Behavior |
|-------|-------------|----------|
| 0 | Empty Space | No rendering, movement passes through |
| 1 | Player Ship | Controlled by user, bottom row only |
| 2 | Meteor | Falls down, destroys on contact |
| 3 | Player Bullet | Moves up, destroys enemies |
| 4 | Enemy UFO | Falls down, awards 1 point |
| 5 | Boss | Falls down, fires bullets, awards 3 points |
| 6 | Boss Bullet | Falls down, damages player |

**Critical Design Decision:**
- Only ONE entity per cell (no overlapping)
- Collision detection is simply checking the value of the target cell
- Movement = changing the value at one position and setting the old position to 0

### Hit Effect System (Lines 53-58)
```cpp
const int MAX_HIT_EFFECTS = 50;
int hitEffectRow[MAX_HIT_EFFECTS] = {0};
int hitEffectCol[MAX_HIT_EFFECTS] = {0};
float hitEffectTimer[MAX_HIT_EFFECTS] = {0.0f};
bool hitEffectActive[MAX_HIT_EFFECTS] = {false};
```

**Purpose**: Visual feedback when entities are destroyed
**How it works:**
1. When a collision destroys an entity, the game finds an inactive slot in these arrays
2. It stores the row/col position and activates the effect
3. Each frame, timers increment
4. After 0.3 seconds, the effect deactivates
5. During rendering, active effects draw explosion sprites

**Linked to**: Rendering loop (lines 1655-1661) and update logic (lines 1352-1363)

### Game State Variables (Lines 28-46)
```cpp
int currentState = STATE_MENU;    // Controls which screen is active
int selectedMenuItem = 0;         // Which menu option is highlighted
int lives = 3;                    // Player health
int score = 0;                    // Points accumulated
int level = 1;                    // Current difficulty tier
const int MAX_LEVEL = 5;          // Win condition
bool isInvincible = false;        // Temporary damage immunity
Clock invincibilityTimer;         // Tracks immunity duration
int bossMoveCounter = 0;          // Determines when bosses fire
```

**How they interact:**
- `currentState` determines which logic block executes in the game loop
- `lives <= 0` triggers `currentState = STATE_GAME_OVER`
- `score >= level * 10` triggers level up sequence
- `isInvincible` prevents multiple hits during the 1-second immunity window

---

## 6. Asset Loading System

### Texture Loading Pattern (Lines 61-227)
Every visual asset follows this pattern:
```cpp
Texture spaceshipTexture;
if (!spaceshipTexture.loadFromFile("assets/images/player.png")) {
    cerr << "Failed to load spaceship texture" << endl;
    return -1;  // Exit program if asset missing
}
Sprite spaceship;
spaceship.setTexture(spaceshipTexture);
spaceship.setScale(
    static_cast<float>(CELL_SIZE) / spaceshipTexture.getSize().x,
    static_cast<float>(CELL_SIZE) / spaceshipTexture.getSize().y
);
```

**Critical Concepts:**

1. **Texture vs Sprite:**
   - `Texture`: The image data loaded from disk (stored in GPU memory)
   - `Sprite`: A drawable object that references a texture and has position/scale

2. **Scaling Logic:**
   - Goal: Make sprite fit exactly in one grid cell (40x40 pixels)
   - Formula: `scale = targetSize / originalSize`
   - Example: If `player.png` is 64x64, scale = 40/64 = 0.625

3. **Why scaling matters:**
   - Assets have varying original sizes
   - Grid cells are uniform (40x40)
   - Scaling ensures visual consistency

**All loaded assets:**
- Player ship, life icon, meteors, enemies, bosses
- Bullets (player red, boss green) + impact effects
- Backgrounds (game area, menu starfield)
- Font for all text rendering

---

## 7. Game State Machine

### State Transitions

**STATE_MENU (Lines 571-635):**
```cpp
if (Keyboard::isKeyPressed(Keyboard::Enter)) {
    if (selectedMenuItem == 0) {  // Start Game
        currentState = STATE_PLAYING;
        // Reset all game variables
        lives = 3; score = 0; level = 1;
        // Clear grid, reposition spaceship
        // Restart all clocks
    }
}
```

**Why reset everything:**
- Starting a new game must wipe previous session data
- Grid must be cleared of leftover entities
- Timers must restart to prevent immediate spawns

**STATE_PLAYING (Lines 698-1371):**
The most complex state - handles:
- Player input (movement, shooting, pause)
- Entity spawning (meteors, enemies, bosses)
- Entity movement with collision detection
- Level progression and victory conditions

**STATE_PAUSED (Lines 1455-1519):**
```cpp
// Draw game state in background (frozen)
// Draw semi-transparent overlay
// Draw pause menu on top
```
**Design choice**: Game remains visible but frozen, giving context to the player

---

## 8. Timing System

### Clock-Based Timing (Lines 414-436)
SFML's `Clock` class measures elapsed time since creation or last restart.

**Movement Cooldown:**
```cpp
Clock moveClock;
Time moveCooldown = milliseconds(100);

// In game loop:
if (moveClock.getElapsedTime() >= moveCooldown) {
    // Allow movement
    if (Keyboard::isKeyPressed(Keyboard::Left)) {
        // Move left
        moveClock.restart();  // Reset timer
    }
}
```

**Why cooldowns exist:**
- Without them, 60 FPS means 60 movements per second (too fast)
- Cooldown of 100ms = max 10 movements/second (playable speed)

**Spawning Timers:**
```cpp
Clock meteorSpawnClock;
float nextSpawnTime = 1.0f + (rand() % 3);  // Random 1-3 seconds

if (meteorSpawnClock.getElapsedTime().asSeconds() >= nextSpawnTime) {
    // Spawn meteor
    meteorSpawnClock.restart();
    nextSpawnTime = 1.0f + (rand() % 3);  // Set next random interval
}
```

**Why random intervals:**
- Predictable spawns are boring
- Randomness creates dynamic challenge
- Each spawn sets a new random interval

**Timer Categories:**
1. **Input Cooldowns**: `moveClock` (100ms), `bulletFireClock` (300ms), `menuClock` (200ms)
2. **Spawn Timers**: `meteorSpawnClock`, `enemySpawnClock`, `bossSpawnClock`
3. **Movement Timers**: `meteorMoveClock`, `enemyMoveClock`, `bulletMoveClock`
4. **Effect Timers**: `hitEffectClock`, `invincibilityTimer`, `levelUpBlinkClock`

---

## 9. The Game Loop

### Structure (Lines 440-1850)
```cpp
while (window.isOpen()) {
    // 1. EVENT POLLING
    Event event;
    while (window.pollEvent(event)) {
        if (event.type == Event::Closed)
            window.close();
    }
    
    // 2. STATE LOGIC (different for each state)
    if (currentState == STATE_MENU) { /* menu logic */ }
    else if (currentState == STATE_PLAYING) { /* game logic */ }
    // ... other states
    
    // 3. RENDERING
    window.clear(Color(40, 40, 40));
    if (currentState == STATE_MENU) { /* draw menu */ }
    else if (currentState == STATE_PLAYING) { /* draw game */ }
    // ... other renders
    window.display();
    
    sleep(milliseconds(50));  // Small delay for game speed
}
```

**Execution Flow (60 times per second):**
1. **Event Polling**: Check if user closed window
2. **Update Logic**: Based on `currentState`, execute appropriate game logic
3. **Rendering**: Draw everything relevant to current state
4. **Display**: Swap buffers (show what was drawn)
5. **Sleep**: Tiny delay to fine-tune game speed

---

## 10. State-Specific Logic

### Playing State Breakdown (Lines 698-1371)

#### Input Handling (Lines 700-751)
**Player Movement:**
```cpp
if (moveClock.getElapsedTime() >= moveCooldown) {
    if (Keyboard::isKeyPressed(Keyboard::Left) && spaceshipCol > 0) {
        grid[ROWS - 1][spaceshipCol] = 0;  // Clear old position
        spaceshipCol--;                     // Update column variable
        grid[ROWS - 1][spaceshipCol] = 1;  // Set new position
        moveClock.restart();
    }
}
```

**Why three steps:**
1. Clear old grid position (set to 0)
2. Update the `spaceshipCol` tracking variable
3. Set new grid position (set to 1)

**Shooting:**
```cpp
if (Keyboard::isKeyPressed(Keyboard::Space) && bulletFireClock.getElapsedTime() >= bulletFireCooldown) {
    int bulletRow = ROWS - 2;  // One row above spaceship
    if (grid[bulletRow][spaceshipCol] == 0) {  // Only if empty
        grid[bulletRow][spaceshipCol] = 3;  // Place bullet
    }
    bulletFireClock.restart();
}
```

**Key detail**: Bullet spawns only if the cell above is empty (prevents stacking)

#### Spawning System (Lines 753-792)

**Enemy Spawn Difficulty Scaling:**
```cpp
float baseTime = 2.5f - (level * 0.4f);  // Faster at higher levels
float variance = 3.0f - (level * 0.4f);   // Less random at higher levels
if (baseTime < 0.5f) baseTime = 0.5f;    // Minimum spawn time
nextEnemySpawnTime = baseTime + (rand() % (int)variance);
```

**Level 1**: 2.5s base + 0-3s variance = 2.5-5.5s between spawns
**Level 5**: 0.5s base + 0-1s variance = 0.5-1.5s between spawns (much faster!)

**Boss Spawning Condition:**
```cpp
if (level >= 3 && bossSpawnClock.getElapsedTime().asSeconds() >= nextBossSpawnTime) {
    // Spawn boss
}
```
**Design**: Bosses only appear after Level 3 to introduce new challenge mid-game

---

## 11. Entity Movement & Collision

### Movement Pattern: Bottom-to-Top Iteration
```cpp
for (int r = ROWS - 1; r >= 0; r--) {  // Start from bottom
    for (int c = 0; c < COLS; c++) {
        if (grid[r][c] == 2) {  // Found a meteor
            // Move it down
        }
    }
}
```

**Why bottom-to-top:**
- Prevents processing the same entity twice in one frame
- Example: If we iterate top-to-bottom and move an entity from row 5 to row 6, when we reach row 6, we'd move it again to row 7 (wrong!)

### Collision Detection Example: Meteor Movement (Lines 795-823)
```cpp
if (grid[r][c] == 2) {  // Found meteor at position [r][c]
    if (r == ROWS - 1) {  // At bottom edge
        grid[r][c] = 0;   // Remove meteor
    }
    else {
        grid[r][c] = 0;  // Clear current position
        
        if (grid[r + 1][c] == 0) {  // Next cell is empty
            grid[r + 1][c] = 2;      // Move meteor down
        }
        else if (grid[r + 1][c] == 1) {  // Next cell has player
            if (!isInvincible) {
                lives--;
                isInvincible = true;
                invincibilityTimer.restart();
            }
            // Don't place meteor (it's destroyed)
        }
        else if (grid[r + 1][c] == 3) {  // Next cell has bullet
            grid[r + 1][c] = 0;  // Destroy both (leave cell empty)
            // Trigger hit effect
        }
    }
}
```

**Collision Logic:**
1. Check the cell you're moving into
2. If empty (0): Move there
3. If player (1): Deal damage, apply invincibility
4. If bullet (3): Destroy both entities
5. Always clear the old position first

### Boss Firing Mechanism (Lines 1058-1098)
```cpp
bossMoveCounter++;  // Increment on each movement

int firingInterval;
if (level == 3) firingInterval = 3;      // Fire every 3 moves
else if (level == 4) firingInterval = 2; // Fire every 2 moves
else firingInterval = 1;                 // Fire almost every move (level 5)

if (bossMoveCounter >= firingInterval) {
    // Find all bosses and fire bullets
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (grid[r][c] == 5) {  // Found a boss
                int bulletRow = r + 1;
                if (grid[bulletRow][c] == 0) {
                    grid[bulletRow][c] = 6;  // Place boss bullet
                }
            }
        }
    }
    bossMoveCounter = 0;  // Reset counter
}
```

**How it scales with difficulty:**
- Level 3: Boss fires every 3rd time it moves
- Level 4: Boss fires every 2nd time it moves
- Level 5: Boss fires almost every time it moves

**Why this matters:** Creates escalating difficulty as player progresses

### Player Bullet Movement (Lines 1165-1337)
**Special behavior: Moves UP instead of down**
```cpp
for (int r = 0; r < ROWS; r++) {  // Top-to-bottom for upward movement
    for (int c = 0; c < COLS; c++) {
        if (grid[r][c] == 3) {  // Found player bullet
            if (r == 0) {  // At top edge
                grid[r][c] = 0;  // Remove bullet
            }
            else {
                grid[r][c] = 0;  // Clear current position
                
                if (grid[r - 1][c] == 4) {  // Hit enemy
                    score += 1;
                    grid[r - 1][c] = 0;  // Destroy both
                    
                    // Check for level up
                    if (score >= level * 10) {
                        level++;
                        // Clear grid, reset score
                        currentState = STATE_LEVEL_UP;
                    }
                }
                else if (grid[r - 1][c] == 5) {  // Hit boss
                    score += 3;  // Bosses worth more
                    // Same level up logic
                }
            }
        }
    }
}
```

**Level Up Trigger:**
1. Player destroys enemy → `score++`
2. Check if `score >= level * 10`
3. If true: Increment level, clear all entities, show "LEVEL UP!" screen
4. After 2 seconds, return to gameplay with increased difficulty

---

## 12. Rendering Pipeline

### Rendering Order (Lines 1522-1850)
```cpp
window.clear(Color(40, 40, 40));  // Dark gray background

if (currentState == STATE_PLAYING) {
    // 1. Draw background (covers whole grid area)
    window.draw(background);
    
    // 2. Draw grid border
    window.draw(gameBox);
    
    // 3. Draw all entities by scanning the grid
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (grid[r][c] == 1) {  // Spaceship
                spaceship.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                window.draw(spaceship);
            }
            else if (grid[r][c] == 2) {  // Meteor
                meteor.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                window.draw(meteor);
            }
            // ... all entity types
        }
    }
    
    // 4. Draw hit effects (explosion animations)
    for (int i = 0; i < MAX_HIT_EFFECTS; i++) {
        if (hitEffectActive[i]) {
            bulletHit.setPosition(MARGIN + hitEffectCol[i] * CELL_SIZE, 
                                  MARGIN + hitEffectRow[i] * CELL_SIZE);
            window.draw(bulletHit);
        }
    }
    
    // 5. Draw UI (score, lives, level)
    window.draw(title);
    window.draw(scoreText);
    // ... other UI elements
}

window.display();  // Show everything drawn this frame
```

**Why this order matters:**
1. Background first (back layer)
2. Entities next (middle layer)
3. Effects on top (explosions should be visible over everything)
4. UI last (front layer, always visible)

### Position Calculation
```cpp
pixelX = MARGIN + col * CELL_SIZE
pixelY = MARGIN + row * CELL_SIZE
```

**Example:**
- Grid position: `[5][7]` (row 5, col 7)
- Pixel position: `MARGIN + 7 * 40, MARGIN + 5 * 40` = `(320, 240)` pixels

### Invincibility Blink Effect (Lines 1615-1621)
```cpp
if (grid[r][c] == 1) {  // Spaceship
    spaceship.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
    
    // Only draw if NOT invincible OR if current blink state is visible
    if (!isInvincible || ((int)(invincibilityTimer.getElapsedTime().asMilliseconds() / 100) % 2 == 0)) {
        window.draw(spaceship);
    }
}
```

**How blink works:**
1. Get elapsed milliseconds since hit (e.g., 350ms)
2. Divide by 100 (3.5) → cast to int (3)
3. Modulo 2 (3 % 2 = 1)
4. If result is 0: Draw spaceship
5. If result is 1: Skip drawing (invisible for that frame)

**Result:** Spaceship flickers on/off every 100ms during invincibility period

---

## 13. How Everything Links Together

### The Complete Game Cycle

```
┌─────────────────────────────────────────────────────────────┐
│                    GAME STARTS                               │
│                    main() called                             │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│              INITIALIZATION PHASE                            │
│  • Seed random generator (srand)                             │
│  • Create window (1180x1000)                                 │
│  • Initialize variables (lives=3, score=0, level=1)          │
│  • Create grid[23][15] = {0}                                 │
│  • Load all textures and create sprites                      │
│  • Create all text objects for UI                            │
│  • Initialize all clocks                                     │
│  • Set currentState = STATE_MENU                             │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                  MAIN GAME LOOP                              │
│              while (window.isOpen())                         │
└────────────────────────┬────────────────────────────────────┘
                         │
         ┌───────────────┴───────────────┐
         │                               │
         ▼                               ▼
┌──────────────────┐          ┌────────────────────┐
│ EVENT POLLING    │          │  STATE LOGIC       │
│ • Window closed? │──────────▶│  Based on         │
│                  │          │  currentState:     │
└──────────────────┘          │                    │
                              │  • STATE_MENU      │
                              │  • STATE_PLAYING   │
                              │  • STATE_PAUSED    │
                              │  • STATE_GAME_OVER │
                              │  • etc.            │
                              └────────┬───────────┘
                                       │
                                       ▼
                              ┌────────────────────┐
                              │   RENDERING        │
                              │  • Clear screen    │
                              │  • Draw based on   │
                              │    currentState    │
                              │  • Display frame   │
                              └────────┬───────────┘
                                       │
                                       ▼
                              ┌────────────────────┐
                              │  Sleep(50ms)       │
                              │  Loop continues    │
                              └────────┬───────────┘
                                       │
                                       └──────────┐
                                                  │
                  (Loops back 60 times/second) ◀──┘
```

### STATE_PLAYING Deep Dive (The Core Gameplay Loop)

```
STATE_PLAYING Logic Flow (executed every frame):
═══════════════════════════════════════════════

1. INPUT HANDLING
   ├─ Check if P pressed → Switch to STATE_PAUSED
   ├─ Check moveClock cooldown
   │  └─ If Left/Right pressed → Update grid, move spaceship
   └─ Check bulletFireClock cooldown
      └─ If Space pressed → Place bullet at grid[ROWS-2][spaceshipCol]

2. ENTITY SPAWNING (Time-based)
   ├─ Check meteorSpawnClock
   │  └─ If elapsed >= nextSpawnTime → Place meteor at grid[0][randomCol]
   ├─ Check enemySpawnClock
   │  └─ If elapsed >= nextEnemySpawnTime → Place enemy at grid[0][randomCol]
   └─ Check bossSpawnClock (only if level >= 3)
      └─ If elapsed >= nextBossSpawnTime → Place boss at grid[0][randomCol]

3. ENTITY MOVEMENT & COLLISION
   ├─ Meteor Movement (every 0.833 seconds)
   │  └─ For each meteor: Try move down, check collisions
   ├─ Enemy Movement (speed based on level)
   │  └─ For each enemy: Try move down, check collisions, award points
   ├─ Boss Movement (speed based on level)
   │  ├─ For each boss: Try move down, check collisions
   │  └─ Increment bossMoveCounter, fire bullets based on interval
   ├─ Boss Bullet Movement (twice as fast as boss)
   │  └─ For each boss bullet: Move down, check player collision
   └─ Player Bullet Movement (very fast, 0.05s)
      └─ For each bullet: Move up, check enemy/boss collision, level up logic

4. EFFECT UPDATES
   ├─ Update hit effect timers
   │  └─ Deactivate effects after 0.3 seconds
   └─ Update invincibility timer
      └─ Set isInvincible = false after 1.0 second

5. STATE TRANSITIONS
   ├─ If lives <= 0 → currentState = STATE_GAME_OVER
   ├─ If score >= level * 10 → currentState = STATE_LEVEL_UP
   └─ If level >= MAX_LEVEL && score >= threshold → currentState = STATE_VICTORY
```

### Data Flow: How Grid Changes Affect Everything

```
GRID UPDATE TRIGGERS VISUAL CHANGES:
════════════════════════════════════

User presses LEFT key
    ↓
moveClock check passes (100ms elapsed)
    ↓
grid[ROWS-1][spaceshipCol] = 0  (clear old)
spaceshipCol--                   (update variable)
grid[ROWS-1][spaceshipCol] = 1  (set new)
    ↓
RENDERING PHASE:
    ↓
Scan grid at [ROWS-1][spaceshipCol-1]
    ↓
Find value = 1 (spaceship)
    ↓
Calculate pixel position:
    x = MARGIN + (spaceshipCol-1) * 40
    y = MARGIN + (ROWS-1) * 40
    ↓
Set spaceship sprite position
    ↓
Draw spaceship at new position
    ↓
Player sees spaceship moved left
```

### Level Progression Flow

```
LEVEL UP SEQUENCE:
═════════════════

Player bullet hits enemy
    ↓
score++ (now score = 10, level = 1)
    ↓
Check: score >= level * 10?  (10 >= 10? YES)
    ↓
level++ (now level = 2)
score = 0
bossMoveCounter = 0
    ↓
Clear entire grid (except spaceship)
    ↓
Reset spaceship to center
    ↓
currentState = STATE_LEVEL_UP
levelUpTimer.restart()
    ↓
RENDERING: Show "LEVEL UP!" with blink effect
    ↓
Wait 2 seconds (levelUpTimer check)
    ↓
currentState = STATE_PLAYING
    ↓
GAMEPLAY RESUMES WITH INCREASED DIFFICULTY:
    ├─ Enemy spawn time reduced
    ├─ Enemy movement speed increased
    └─ Bosses spawn if level >= 3
```

### Collision Resolution Priority

```
ENTITY INTERACTION MATRIX:
═════════════════════════

When Entity A tries to move into Entity B's cell:

           │ Empty │ Player │ Meteor │ P.Bullet │ Enemy │ Boss │ B.Bullet
═══════════╪═══════╪════════╪════════╪══════════╪═══════╪══════╪═════════
Meteor     │ Move  │ Damage │ Pass   │ Destroy  │ Pass  │ Pass │ Pass
Enemy      │ Move  │ Damage │ Pass   │ +1 Score │ Pass  │ Pass │ Pass
Boss       │ Move  │ Damage │ Pass   │ +3 Score │ Pass  │ Pass │ Pass
P.Bullet   │ Move  │ Block  │Destroy │ Pass     │+1Score│+3Scor│ Destroy
B.Bullet   │ Move  │ Damage │ Pass   │ Destroy  │ Pass  │ Pass │ Pass

Legend:
- Move: Entity moves into cell
- Damage: Player loses life, entity destroyed
- Pass: Entity phases through
- Destroy: Both entities destroyed
- +X Score: Entity destroyed, player gains X points
- Block: Bullet cannot move there
```

### Memory & Performance Considerations

**Why Grid-Based Design is Efficient:**
1. **Fixed Memory**: `grid[23][15]` = 345 integers (1,380 bytes) regardless of entity count
2. **O(1) Collision Detection**: Just check `grid[targetRow][targetCol]`
3. **Predictable Performance**: Always scan exactly 345 cells per frame

**Alternative (Object-Oriented) Would Require:**
- Dynamic arrays of enemy/bullet objects
- O(n²) collision checks (check each bullet against each enemy)
- Memory allocation/deallocation overhead

### Timing Synchronization

```
FRAME TIMING AT 60 FPS:
═══════════════════════

Frame Duration: ~16.67ms

Within each frame:
├─ Event Polling: ~0.1ms
├─ State Logic:
│  ├─ Input checks: ~0.5ms
│  ├─ Spawning logic: ~1ms
│  ├─ Movement & collision: ~3-5ms (depends on entity count)
│  └─ Effect updates: ~0.5ms
├─ Rendering:
│  ├─ Clear screen: ~0.5ms
│  ├─ Draw background: ~1ms
│  ├─ Draw entities: ~3-8ms (depends on entity count)
│  └─ Draw UI: ~1ms
└─ Display & sleep: ~50ms (artificial delay)

Total: ~60-70ms per frame (intentionally slow for classic game feel)
```

### Complete State Dependency Graph

```
VARIABLE DEPENDENCIES:
═════════════════════

currentState (Master Controller)
    ├─ Controls which logic executes
    ├─ Controls what gets rendered
    └─ Modified by:
        ├─ User input (Enter, P, Esc)
        ├─ lives <= 0
        └─ score >= threshold

lives
    ├─ Decreased by: Meteor hit, Enemy hit, Boss hit, Boss bullet hit
    ├─ Protected by: isInvincible flag
    └─ Triggers: STATE_GAME_OVER when <= 0

score
    ├─ Increased by: Destroying enemies (+1), Destroying bosses (+3)
    └─ Triggers: Level up when >= level * 10

level
    ├─ Increased by: Reaching score threshold
    ├─ Affects: Enemy speed, spawn rate, boss appearance, firing rate
    └─ Triggers: STATE_VICTORY when >= MAX_LEVEL

grid[ROWS][COLS] (Core Game State)
    ├─ Modified by: Spawning, Movement, Collision resolution
    ├─ Read by: Rendering loop, Collision detection
    └─ Cleared on: Game start, Level up

isInvincible
    ├─ Set true on: Any damage
    ├─ Set false after: 1 second (invincibilityTimer)
    └─ Affects: Damage prevention, Rendering (blink effect)

All Clocks
    ├─ Restarted on: Specific actions (movement, spawning, shooting)
    ├─ Read continuously: To check if cooldowns/intervals elapsed
    └─ Control: Game timing, difficulty scaling, visual effects
```

---

## Summary: The Complete Picture

The game is a **state-driven, grid-based arcade shooter** where:

1. **The grid is the single source of truth** - all game logic revolves around reading and writing integer values to `grid[ROWS][COLS]`

2. **Timing controls everything** - SFML Clocks manage when entities spawn, move, and shoot, creating difficulty progression

3. **State machine provides structure** - `currentState` determines which code executes and what displays on screen

4. **Collision is implicit** - No complex math; just check the value of the target cell before moving

5. **Rendering is reactive** - Every frame, scan the grid and draw sprites where entities exist

6. **Difficulty scales naturally** - As `level` increases, spawn rates increase and movement speeds up through mathematical formulas

7. **Everything connects through shared variables** - `lives`, `score`, `level`, and `grid` are accessed by multiple logic blocks, creating emergent gameplay

The beauty of this design is its **simplicity and directness**: there's no hidden complexity, no abstraction layers. The entire game state is visible in a few key variables, making it easy to understand, debug, and modify.
