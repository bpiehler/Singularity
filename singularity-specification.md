# Product Specification: Singularity (Pebble Idle Game)

## 1. Product Overview
**Singularity** is an incremental (idle) game for the Pebble smartwatch platform. The player begins with a single milligram of mass and, through manual interaction and gravitational attraction, accumulates enough mass to form cosmic bodies. The ultimate goal is to reach a state of infinite density—triggering a "Big Bang" prestige reset.

## 2. Target Platform & Constraints
* **Hardware:** All Pebble models (Aplite, Basalt, Chalk, Diorite, Emery, Gabbro).
* **SDK:** Pebble SDK 4.x (C-based).
* **Display:** 144x168 (rect) or 180x180 (round), 1-bit or 8-color.
* **Input:** 
    * **SELECT:** Open Shop (consistent with Pebble system patterns).
    * **DOWN:** Raw click (Tapping) and AppTimer-based Auto-Tap (long-press).
    * **UP:** Open Stats / Cosmic Ledger.
* **Activity:** `HealthService` integration (1 step = 100 mg of Mass).

---

## 3. Core Mechanics

### A. The Resource Loop
* **Primary Currency:** **Mass** (Stored as a double).
* **Passive Generation:** **Gravity** (Mass earned per second).
* **Prestige Currency:** **Cosmic Dust** (+10% global multiplier per dust).

### B. Progression Tiers (9 Tiers)
1. **Pebble:** 100 mg
2. **Rock:** 1.0M mg
3. **Boulder:** 1.0e10 mg
4. **Mountain:** 1.0e14 mg
5. **Asteroid:** 1.0e18 mg
6. **Moon:** 1.0e22 mg
7. **Planet:** 1.0e26 mg
8. **Gas Giant:** 1.0e30 mg
9. **Star:** 1.0e34 mg

### C. The Prestige Mechanic: The Big Bang
Once the player reaches **1.0e36 mg**, they can trigger a **Collapse**.
* **Action:** Select "THE BIG BANG" row at the bottom of the Shop menu.
* **Reward:** Cosmic Dust (scaled by `sqrt(Mass / Threshold)`).

---

## 4. UI/UX Design
* **Main Screen:** Decoupled UI that refreshes every 1s to prevent Watchdog timeouts.
* **Visual Circle:** Grows logarithmically based on mass toward the prestige threshold.
* **Shop:** Accessible via SELECT; supports "Buy Max" via Long-SELECT on a tier.
* **Stats:** Accessible via UP; tracks playtime, singularities, and peak mass.

---

## 5. Development & Testing
* **God Mode (Debug):** 
    * **Hold DOWN (500ms):** Instantly sets mass to the Prestige Threshold.
    * **Hold UP (500ms):** Grants a large mass boost (6 hours of gravity).
* **Auto-Tap (Planned):** Long-press DOWN will eventually transition from God Mode to a rapid 200ms auto-tap.
