# Product Specification: Singularity (Pebble Idle Game)

## 1. Product Overview
**Singularity** is an incremental (idle) game for the Pebble smartwatch platform. The player begins with a single milligram of mass and, through manual interaction and gravitational attraction, accumulates enough mass to form cosmic bodies. The ultimate goal is to reach a state of infinite density—triggering a "Big Bang" prestige reset.

## 2. Target Platform & Constraints
* **Hardware:** All Pebble models (Aplite, Basalt, Chalk, Diorite, Emery, Gabbro).
* **SDK:** Pebble SDK 4.x (C-based).
* **Display:** 144x168 (rect) or 180x180 (round), 1-bit or 8-color.
* **Input:** Raw Select button click (Tapping) and AppTimer-based Auto-Tap.
* **Activity:** `HealthService` integration (1 step = 100 mg of Mass).

---

## 3. Core Mechanics

### A. The Resource Loop
* **Primary Currency:** **Mass** (Stored as a double).
* **Passive Generation:** **Gravity** (Mass earned per second).
* **Prestige Currency:** **Cosmic Dust** (+20% global multiplier per dust).

### B. Progression Tiers (9 Tiers)
1. **Pebble:** 100 mg
2. **Rock:** 2.5k mg
3. **Boulder:** 50k mg
4. **Hill:** 1.0M mg
5. **Mountain:** 50M mg
6. **Planet:** 2.0B mg
7. **Star:** 1.0T mg
8. **Galaxy:** 500T mg
9. **Supercluster:** 1.0e16 mg

### C. The Prestige Mechanic: The Big Bang
Once the player reaches **1.0e16 mg**, they can trigger a **Collapse**.
* **Action:** 5-second long-press on Select.
* **Reward:** 1 Cosmic Dust (plus bonus based on excess mass).

---

## 4. UI/UX Design (MVP Status)
* **Main Screen:** Decoupled UI that refreshes every 1s to prevent Watchdog timeouts.
* **Visual Circle:** Grows based on progress toward the next affordable tier.
* **Shop:** Accessible via Up/Down; supports "Buy Max" via Long-Select.

---

## 5. Implementation Roadmap

* **Phase 1-5: [COMPLETE]** (Logic, Skeleton, Persistence, Visuals, Health).
* **Phase 6: [CURRENT]** UX Polish, Sound, and Visual Effects.
* **Phase 7: [TODO]** Balance Tuning and Milestone Unlocks (Every 25 units).
