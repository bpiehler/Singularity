# Product Specification: Singularity (Pebble Idle Game)

## 1. Product Overview
**Singularity** is an incremental (idle) game for the Pebble smartwatch platform. The player begins with a single milligram of mass and, through manual interaction and gravitational attraction, accumulates enough mass to form pebbles, rocks, and eventually cosmic bodies. The ultimate goal is to reach a state of infinite density—a Singularity—triggering a "Big Bang" prestige reset to begin the cycle again with increased efficiency.

## 2. Target Platform & Constraints
* **Hardware:** Pebble Classic, Pebble Time, Pebble Time Round, and modern variants (e.g., hardware with touch support).
* **SDK:** Pebble SDK 3.0/4.0 (C-based).
* **Display:** 144x168 (standard) or 180x180 (round), 1-bit or 8-color (dithered).
* **Input:**
    * **Physical:** Select button (Primary Action).
    * **Touch:** Screen Tap (Primary Action).
    * **Activity:** Step counting via `HealthService` (Configurable as "Steps = Taps").

---

## 3. Core Mechanics

### A. The Resource Loop
* **Primary Currency:** **Mass** (Stored as a double to handle large values).
* **Passive Generation:** **Gravity** (Mass earned per second).
* **Manual Action:** **Compression** (Button press/Tap/Step). Initially, 1 tap = 1 mg of Mass.

### B. Progression Tiers
As players accumulate Mass, they spend it to "compress" existing mass into larger, more gravity-dense objects.

| Tier | Name | Base Cost | Gravity Yield (per sec) |
| :--- | :--- | :--- | :--- |
| 1 | **Pebble** | 100 mg | +1 mg |
| 2 | **Rock** | 2,500 mg | +20 mg |
| 3 | **Boulder** | 50,000 mg | +500 mg |
| 4 | **Hill** | 1,000,000 mg | +12,000 mg |
| 5 | **Mountain** | 50,000,000 mg | +750,000 mg |
| 6 | **Planet** | 2,000,000,000 mg | +40,000,000 mg |

### C. The Prestige Mechanic: The Big Bang
Once the player reaches **Black Hole** status (defined by a Mass threshold, e.g., 10^15 mg), they can trigger a **Collapse**.
* **Action:** 5-second long-press on Select.
* **Visual:** Screen inversion and pixel-warping toward the center.
* **Reward:** Permanent "Cosmic Dust" currency that provides a multiplicative bonus to Gravity in the next run (e.g., +10% Gravity per Dust).

---

## 4. UI/UX Design

### A. Main Game Screen (The "Well")
* **Top Header:** Total Mass (displayed with shorthand: 1k, 1M, 1B, 1T).
* **Center Graphic:** A dithered circle representing the current "Center of Mass."
    * **Visual Growth:** The circle grows slightly every 10% toward the next tier cost.
    * **Visual Polish:** Near the "Black Hole" tier, the UI elements (Gravity readout, etc.) should begin to "lean" or drift toward the center of the screen.
* **Bottom Footer:** Current Gravity per second (e.g., `G: 1.2k/s`).

### B. The Shop/Menu
* Accessed via **Up/Down** buttons.
* Lists tiers, current count owned, and the cost of the next unit.
* "Buy" triggered by **Select/Tap**.

### C. Haptics (The "Vibe" Policy)
* **Standard Play:** No vibration.
* **Tier Unlock:** Short double-pulse.
* **Big Bang:** A continuous, escalating vibration (`VibePattern`) until the screen resets.

---

## 5. Technical Requirements

### A. Math Logic
* **Cost Scaling Formula:** `C = B * 1.15^n`
    * `C`: Current Cost
    * `B`: Base Cost
    * `1.15`: Growth Factor
    * `n`: Number of items already owned.
* **Double Support:** Use `double` or a fixed-point math library to avoid integer overflow at high tiers.

### B. Persistence & Background Progress
* **API:** `persist_write_data` / `persist_read_data`.
* **Logic:** Save current `Mass` and `Timestamp` on `deinit`.
* **Offline Gains:** On `init`, calculate: `(CurrentTime - SavedTime) * Gravity`. Reward the player with a "Mass Gained While Away" splash screen.

### C. Health Integration
* **Config Toggle:** "Steps count as Taps."
* **Logic:** Use `HealthService` to subscribe to step count updates. Every X steps recorded, call the `add_mass()` function.

---

## 6. Implementation Plan Roadmap

1. **Phase 1 (Logic):** Develop a headless JS/C model to verify the 1.15x scaling doesn't "break" the game pace over 48 hours.
2. **Phase 2 (Skeleton):** Create a Pebble App with basic text layers showing Mass increasing via buttons.
3. **Phase 3 (Persistence):** Implement the save/load and offline math.
4. **Phase 4 (Visuals):** Add the dithered "Mass Center" and the Shop menu.
5. **Phase 5 (Health/Config):** Integrate the Step-counter and Clay-based configuration page.
