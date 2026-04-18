# Singularity: Development & Debugging Lessons

This document captures critical technical lessons learned during the development of the Singularity Pebble app, specifically regarding ARM hardware constraints, SDK limitations, and performance optimization.

---

## 1. Hardware & Memory Constraints

### **8-Byte Alignment for Doubles**
*   **Issue:** The Pebble's ARM processor triggers a `HardFault` (silent crash) if a `double` (64-bit) is accessed on an unaligned memory boundary.
*   **Lesson:** Always move `double` fields to the top of structs or use `__attribute__((aligned(8)))`.
*   **Resolution:** Reordered `GameState` and applied explicit alignment to the static state variable.

### **Watchdog Timer (The "Minutes Later" Crash)**
*   **Issue:** If the main thread stays busy for too long (blocking the event loop), the OS assumes the app has hung and reboots the watch.
*   **Lesson:** Avoid expensive math (`pow()`, `log()`) inside drawing routines or high-frequency timers.
*   **Resolution:** Implemented **Gravity Caching**. We calculate gravity once when a purchase is made and store it, rather than recalculating 5+ times per second.

### **Heavy Disk I/O**
*   **Issue:** `persist_write_data` can take up to 1 second in the emulator. If this runs inside `tick_handler`, it causes a "skipped tick" and triggers the Watchdog.
*   **Lesson:** Never save to disk inside the heartbeat loop.
*   **Resolution:** Moved auto-save to a dedicated `AppTimer` running every 5 minutes.

---

## 2. UI & Interaction Polish

### **Decoupling Engine from UI**
*   **Issue:** Calling `update_display()` (which triggers a screen redraw) inside a 100ms or 200ms timer causes input lag and reboots.
*   **Lesson:** Redraw the screen no more than once per second.
*   **Resolution:** High-frequency clicks update the `mass` in memory instantly, but the visual "UI Catch-up" is deferred to the 1-second `tick_handler`.

### **Input Conflict (Repeating vs Long-Click)**
*   **Issue:** The SDK `single_repeating_click` and `long_click` conflict if registered on the same button.
*   **Lesson:** For complex overlapping inputs (Auto-Tap + Prestige), use `window_raw_click_subscribe` and a manual timer to track hold duration.
*   **Resolution:** Implemented a manual `s_hold_time_ms` tracker to separate single taps from the 5-second Big Bang hold.

### **The "Double-Tap" Bug**
*   **Issue:** Firing a repeat timer immediately on down-press causes many human clicks to register twice.
*   **Lesson:** Add an initial delay (e.g., 500ms) before the "auto-fire" logic begins.

---

## 3. SDK & Service Quirks

### **Health Service Race Conditions**
*   **Issue:** Calling `health_service_sum_today` during `init()` or with invalid time ranges (`time(NULL)` to `time(NULL)`) causes hangs.
*   **Lesson:** 
    1.  Always add `"capabilities": ["health"]` to `package.json`.
    2.  Use `time_start_of_today()` for range checks.
    3.  Defer first retrieval until the first `HealthEvent` fires.

### **MenuLayer Drawing**
*   **Issue:** `menu_cell_basic_draw` is restrictive for custom colors or B&W dithering.
*   **Lesson:** For a "disabled" or "grayed out" look, use `graphics_draw_text` manually. On B&W screens, use `GCompOpClear` as a mask to simulate gray via a checkerboard dither.

---

## 5. Focus & Lifecycle

### **Focus Catch-up Logic**
*   **Issue:** When a notification appears or the emulator loses focus, the `TickTimer` pauses. This could cause the player to lose mass accumulation.
*   **Lesson:** Use `AppFocusService` to detect when the app returns to the foreground.
*   **Resolution:** Implemented a `focus_handler` that triggers an immediate `game_state_apply_offline_gains()`. This ensures that every second of "lost focus" time is instantly accounted for as soon as the app is visible again.

---

## 4. Portability & Math

### **Safe Scientific Notation**
*   **Issue:** Standard `snprintf` with `%e` is inconsistent across Pebble firmware versions and can be slow.
*   **Lesson:** Use a custom `format_mass` utility to manually extract the mantissa and exponent.
*   **Resolution:** Implemented a robust `format_mass` with `isnan` and `isinf` guards to prevent infinite loops if math overflows.

### **Geometric Series for "Buy Max"**
*   **Issue:** An `O(n)` loop for bulk-buying thousands of units causes a UI freeze.
*   **Lesson:** Use the closed-form geometric series formula for `O(1)` cost calculations.
