# Singularity: Development & Debugging Lessons

This document captures critical technical lessons learned during the development of the Singularity Pebble app, specifically regarding ARM hardware constraints, SDK limitations, and performance optimization.

---

## 1. Hardware & Memory Constraints

### **8-Byte Alignment for Doubles**
*   **Issue:** The Pebble's ARM processor triggers a `HardFault` (silent crash) if a `double` (64-bit) is accessed on an unaligned memory boundary.
*   **Lesson:** Always move `double` fields to the top of structs or use `__attribute__((aligned(8)))`.
*   **Resolution:** Reordered `GameState` and applied explicit alignment to the static state variable.

### **The Zero-Cast Rule (Large Double -> Int)**
*   **Issue:** Casting a `double` value greater than $2.1 \times 10^9$ (INT_MAX) directly to an `int` or `long` triggers an immediate hardware exception on ARM.
*   **Lesson:** **NEVER** cast mass-scale variables to integers.
*   **Resolution:** Implement manual whole/fractional extraction using `modf()` or custom string formatting (e.g., scientific notation) to display large numbers safely.

### **Stack Preservation (UI Thread)**
*   **Issue:** The Pebble UI thread has a very small stack (< 4KB). Large local string buffers (e.g., `char buf[128]`) inside repetitive drawing callbacks can trigger a stack overflow.
*   **Lesson:** Use `static` buffers for string formatting inside `MenuLayer` or `LayerUpdateProc` callbacks.
*   **Resolution:** Moved all shop row buffers to `static char` to ensure they live in the data segment rather than the stack.

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

### **Input Conflict (Raw vs Long-Click)**
*   **Issue:** The Pebble SDK `window_single_repeating_click_subscribe` (or `window_raw_click_subscribe`) and `window_long_click_subscribe` conflict if registered on the same button. The behavior is undefined and often leads to the long-click failing or the button "locking up."
*   **Lesson:** Use `window_raw_click_subscribe` and a manual `AppTimer` to handle both taps and holds on a single button.
*   **Resolution:** Unified the DOWN button handler into a single raw interface that tracks `s_hold_time_ms` to support both Tapping and "God Mode" (later Auto-tap).

### **Window Lifecycle & Memory Leaks**
*   **Issue:** Setting a window pointer to `NULL` in a `.unload` handler does not actually destroy the window or free its memory.
*   **Lesson:** Use `window_destroy()` for cleanup. However, never call `window_destroy()` on a window that might still be on the window stack.
*   **Resolution:** Implemented a safe `deinit()` pattern for sub-menus (Shop/Stats) that calls `window_stack_remove()` before `window_destroy()`.

---

## 2. UI & Interaction Polish

### **System-Consistent Mapping**
*   **Design Choice:** Moved "Open Shop" to the **SELECT** button. This aligns with standard Pebble apps where the middle button acts as the primary "Action/Menu" button.
*   **Prestige Location:** Moved "Big Bang" initiation to a dedicated row in the Shop menu to prevent accidental resets and provide a clear reward preview.

### **Testing: God Mode**
*   **Utility:** To facilitate rapid balance testing, long-pressing **DOWN** (500ms) instantly triggers the prestige threshold, while long-pressing **UP** grants a large mass boost.

### **Decoupling Engine from UI**
*   **Issue:** Calling `update_display()` (which triggers a screen redraw) inside a 100ms or 200ms timer causes input lag and reboots.
*   **Lesson:** Redraw the screen no more than once per second.
*   **Resolution:** High-frequency clicks update the `mass` in memory instantly, but the visual "UI Catch-up" is deferred to the 1-second `tick_handler`.

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
