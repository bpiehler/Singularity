# Code Review: Singularity — Pebble Idle Game

**Reviewed:** April 18, 2026  
**Branch:** `main` (HEAD: `a611106`)  
**Scope:** Startup logic, event handling, and button click handling — targeting ongoing hangs/crashes  
**SDK Reference:** https://developer.rebble.io/docs/c/

---

## Summary

The codebase is a reasonably well-structured Pebble idle game. The most recent commits addressed a critical struct alignment HardFault and several defensive issues. However, **two bugs remain that directly explain the reported hangs and button click problems** — one is a documented SDK violation, and one is an algorithmic issue. Several additional correctness and safety issues are documented below.

---

## Critical Bugs

### 1. `window_single_repeating_click_subscribe` and `window_long_click_subscribe` on the Same Button (SDK Violation)

**File:** `src/main.c` — `click_config_provider()`

```c
window_single_repeating_click_subscribe(BUTTON_ID_SELECT, 200, select_click_handler);
window_long_click_subscribe(BUTTON_ID_SELECT, 5000, prestige_handler, NULL);
```

**The SDK explicitly states:** "`single_repeating_click` and `long_click` conflict on the same button."

This is the most likely root cause of the reported button click handling issues. The behavior when both are registered is undefined. Depending on the SDK version and platform, the observed effects can include:

- The long-press handler never fires (long press only registers repeated taps)
- The first repeating tap fires then the button "locks up" until released
- The prestige handler fires unexpectedly on rapid taps

**Fix:** You cannot use both on the same button. Choose one of these approaches:

**Option A** — Use `window_raw_click_subscribe` to get raw press/release events and implement both behaviors manually with a timer:

```c
static AppTimer *s_hold_timer = NULL;
static bool s_is_held = false;

static void hold_timer_callback(void *context) {
    s_hold_timer = NULL;
    s_is_held = true;
    // Trigger prestige attempt
    if (s_state.mass >= PRESTIGE_THRESHOLD) {
        game_state_prestige(&s_state);
        update_next_tier_cost();
        update_display();
        vibes_double_pulse();
    }
}

static void select_down_handler(ClickRecognizerRef recognizer, void *context) {
    s_is_held = false;
    s_hold_timer = app_timer_register(5000, hold_timer_callback, NULL);
}

static void select_up_handler(ClickRecognizerRef recognizer, void *context) {
    if (s_hold_timer) {
        app_timer_cancel(s_hold_timer);
        s_hold_timer = NULL;
    }
    if (!s_is_held) {
        // It was a tap — add mass
        s_state.mass += game_state_calculate_tap_strength(&s_state);
    }
}

// In click_config_provider:
window_raw_click_subscribe(BUTTON_ID_SELECT, select_down_handler, select_up_handler, NULL);
```

**Option B** — Drop the repeating subscription and use a simple single-click that the user taps repeatedly. This is simpler but loses the held-tap behavior:

```c
window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
window_long_click_subscribe(BUTTON_ID_SELECT, 5000, prestige_handler, NULL);
```

---

### 2. `game_state_buy_max()` — O(n) Loop with `pow()` Can Hang the App

**File:** `src/game_state.c` — `game_state_buy_max()`

```c
void game_state_buy_max(GameState *state, int i) {
  while (true) {
    double cost = calculate_cost(TIERS[i].base_cost, state->counts[i]);
    if (state->mass >= cost) {
      state->mass -= cost;
      state->counts[i]++;
    } else {
      break;
    }
    if (state->counts[i] > 1000000) break;
  }
}
```

Each iteration calls `calculate_cost()`, which calls `pow(1.15, count)`. `pow()` is expensive in software floating-point on Cortex-M (the Pebble CPU). In mid-to-late game with the cheapest tiers (e.g., Pebbles at base cost 100), this loop can run hundreds of times:

- At mass 1e14 (approaching prestige): ~230 iterations for Pebbles
- If a save is corrupted and loads a very large mass: potentially thousands of iterations

230 `pow()` calls at ~1-5ms each on ARM without FPU = up to 1+ second freeze. This explains hangs when long-pressing tiers in the shop.

**Fix:** Calculate the number of units to buy in O(1) using the geometric series closed form. The total cost to buy `k` units starting from count `n` is:

```
total = base * 1.15^n * (1.15^k - 1) / 0.15
```

Solving for k:
```
k = floor(log(1 + mass * 0.15 / (base * 1.15^n)) / log(1.15))
```

```c
void game_state_buy_max(GameState *state, int i) {
  double base = TIERS[i].base_cost;
  double current_cost = calculate_cost(base, state->counts[i]);
  if (state->mass < current_cost) return;

  // Closed-form: k = floor(log(1 + mass * 0.15 / current_cost) / log(1.15))
  int k = (int)(log(1.0 + state->mass * 0.15 / current_cost) / log(1.15));
  if (k < 1) k = 1;

  // Compute exact total cost for k units (geometric series)
  double total_cost = current_cost * (pow(1.15, k) - 1.0) / 0.15;

  // Verify and adjust for floating-point rounding
  while (total_cost > state->mass && k > 0) {
    k--;
    total_cost = current_cost * (pow(1.15, k) - 1.0) / 0.15;
  }

  if (k > 0 && state->mass >= total_cost) {
    state->mass -= total_cost;
    state->counts[i] += k;
  }
}
```

This is O(1) regardless of how many units are purchased.

---

## High-Severity Issues

### 3. `canvas_update_proc` — Undefined Behavior Casting Large Double to `int`

**File:** `src/main.c` — `canvas_update_proc()`

```c
int radius = 10 + (int)((s_state.mass / goal) * 50);
if (radius > 70) radius = 70;
```

If `s_state.mass` far exceeds `goal` (e.g., at prestige-scale mass with a low `s_next_tier_cost`), the expression `(s_state.mass / goal) * 50` can be a very large floating-point number. Casting a `double` larger than `INT_MAX` to `int` is **undefined behavior in C** — it can produce crashes, garbage values, or unexpected behavior depending on the platform.

The clamp `if (radius > 70)` runs *after* the UB cast, so it doesn't protect against it.

**Fix:** Clamp the ratio before the cast:

```c
double ratio = s_state.mass / goal;
if (ratio > 1.0) ratio = 1.0;
int radius = 10 + (int)(ratio * 50);
```

---

### 4. `health_handler` — `HealthEventSignificantUpdate` Not Handled; Steps Lost After Midnight

**File:** `src/main.c` — `health_handler()`

The handler treats all `HealthEventType` values identically. Per the SDK, `HealthEventSignificantUpdate` signals that **all health data has been invalidated** — this fires at midnight (day rollover) and on certain OS events. When it fires:

- `health_service_sum_today(HealthMetricStepCount)` returns the new day's count (starts at 0)
- `s_last_step_count` still holds the previous day's value (e.g., 8,000)
- `delta = 0 - 8000 = -8000`, filtered by `if (delta > 0)` ✓

But now `s_last_step_count` is **not reset to 0**. On the new day, every subsequent `HealthEventMovementUpdate` will compute `delta = new_steps - 8000`, returning negative or zero until the user has walked more than yesterday's total. The player's steps are silently discarded for the rest of the new day.

**Fix:** Reset the baseline on significant updates:

```c
static void health_handler(HealthEventType event, void *context) {
  #if defined(PBL_HEALTH)
  if (event == HealthEventSignificantUpdate) {
    s_last_step_count = 0;  // Day rollover — reset baseline
  }

  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(
    HealthMetricStepCount, start, end);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    int total_steps = (int)health_service_sum_today(HealthMetricStepCount);
    if (s_last_step_count == 0) {
      s_last_step_count = total_steps;
    } else {
      int delta = total_steps - s_last_step_count;
      if (delta > 0) {
        game_state_add_steps(&s_state, delta);
        update_display();
      }
      s_last_step_count = total_steps;
    }
  }
  #endif
}
```

---

### 5. `health_service_events_subscribe` Return Value Ignored

**File:** `src/main.c` — `init()`

```c
health_service_events_subscribe(health_handler, NULL);
```

Per the SDK: this function **allocates up to 2048 bytes on the app heap** and returns `false` if insufficient heap is available. On memory-constrained platforms (especially aplite, which has very limited RAM), this can silently fail. Steps are then never tracked and there is no diagnostic.

**Fix:**

```c
#if defined(PBL_HEALTH)
if (!health_service_events_subscribe(health_handler, NULL)) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Health subscription failed (out of memory?)");
}
#endif
```

---

## Medium-Severity Issues

### 6. `shop_menu_show()` — `window_destroy` Called on Potentially Stacked Window

**File:** `src/shop_menu.c` — `shop_menu_show()`

```c
if (s_shop_window) {
  window_destroy(s_shop_window);  // Destroys window before checking if it's on the stack
}
```

The SDK warns against calling `window_destroy()` on a window currently on the stack. While this specific code path is unlikely in normal use (the main window's UP/DOWN handlers are inactive when the shop is on top), it is still a latent bug. If `shop_menu_show()` were ever called from a different context (e.g., a timer callback), this would corrupt the window stack.

**Fix:** Use `window_stack_remove()` instead:

```c
if (s_shop_window) {
  window_stack_remove(s_shop_window, false);
  window_destroy(s_shop_window);
  s_shop_window = NULL;
}
```

---

### 7. Service Subscriptions Not Unsubscribed in `deinit()`

**File:** `src/main.c` — `deinit()`

```c
void deinit(void) {
  game_state_save(&s_state);
  window_destroy(s_main_window);
  // Missing: tick_timer_service_unsubscribe()
  // Missing: health_service_events_unsubscribe()
}
```

Per SDK best practices, services should be explicitly unsubscribed on exit. The health subscription allocates 2048 bytes — not unsubscribing means the OS must clean this up rather than the app itself. While Pebble OS will reclaim memory on app exit, explicit cleanup reflects correct resource management and reduces the risk of late-firing callbacks during teardown.

**Fix:**

```c
void deinit(void) {
  tick_timer_service_unsubscribe();
  #if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
  #endif
  game_state_save(&s_state);
  window_destroy(s_main_window);
}
```

---

### 8. Service Subscriptions Should Be in `window_appear` / `window_disappear`, Not `init()`

**File:** `src/main.c` — `init()` and window handlers

Per the SDK lifecycle documentation:
> `.appear` — Start timers, subscribe to services that should only run when visible.  
> `.disappear` — Stop timers, pause activity.

Both `tick_timer_service_subscribe` and `health_service_events_subscribe` are called in `init()`. This means the game accumulates mass and processes health events even when the main window is not visible (e.g., while a system notification is covering the screen, or if another window is pushed). While the current `update_display()` null-checks prevent crashes, this is against recommended SDK patterns and can lead to subtle bugs.

**Recommended structure:**

```c
static void main_window_appear(Window *window) {
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  #if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
  #endif
}

static void main_window_disappear(Window *window) {
  tick_timer_service_unsubscribe();
  #if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
  #endif
}
```

Note: If pausing the game timer while the shop is open is undesirable (you want the game to keep running), using `.load`/`.unload` instead of `.appear`/`.disappear` is acceptable — but the intent should be explicit.

---

### 9. Struct Alignment Change Silently Invalidates Saved Game State

**File:** `src/game_state.h` — `GameState`

The recent fix reordered the struct fields (moving `double mass` and `double dust` before `uint32_t version`) to fix a HardFault. However, any save data written with the old struct layout will be read with the wrong field offsets. The `version` check in `game_state_load()` will fail (version is now at a different byte offset), and `game_state_init()` will be called — silently wiping the player's save.

This is unavoidable given the alignment fix, but players who update the app will lose their progress without explanation. Consider displaying a one-time notification or splash screen explaining the reset, or implementing a migration path.

---

## Low-Severity Issues

### 10. `BUTTON_ID_DOWN` Handler Naming

**File:** `src/main.c` — `click_config_provider()`

```c
window_single_click_subscribe(BUTTON_ID_DOWN, up_click_handler);
```

The DOWN button is subscribed with `up_click_handler`. This is confusing to read. Consider renaming the function to `open_shop_handler` since that's its actual purpose.

---

### 11. `select_click_handler` — Static Local Variable

**File:** `src/main.c` — `select_click_handler()`

```c
static int click_count = 0;
APP_LOG(APP_LOG_LEVEL_DEBUG, "Tap registered (#%d)", ++click_count);
```

`static` local variables in C persist for the lifetime of the program. This works, but static locals inside functions are non-idiomatic in C and can be confusing. If `click_count` is needed for debugging, move it to module scope alongside the other static state.

---

### 12. `format_mass` Buffer Size Hardcoded

**File:** `src/math_utils.c` — `format_mass()`

```c
void format_mass(double mass, char *buffer) {
  if (isnan(mass)) { snprintf(buffer, 16, "NaN"); return; }
```

The buffer size `16` is hardcoded throughout `format_mass`, but the function accepts a `char *` from callers who may provide different buffer sizes. The function signature should accept a `size_t size` parameter (standard C pattern for safe string functions), or at minimum use a `#define FORMAT_MASS_BUFFER_SIZE 16` constant so callers know the minimum required size.

---

## Summary Table

| # | Severity | Issue | File | Explains Reported Bug? |
|---|----------|-------|------|------------------------|
| 1 | Critical | `repeating_click` + `long_click` on same SELECT button (SDK conflict) | `main.c` | ✅ Button click handling |
| 2 | Critical | `game_state_buy_max()` O(n) loop with `pow()` — freezes on long-press | `game_state.c` | ✅ Hang on button click |
| 3 | High | `canvas_update_proc` casts potentially huge double to int (UB) | `main.c` | ⚠️ Potential crash |
| 4 | High | `HealthEventSignificantUpdate` not handled — steps lost after midnight | `main.c` | ❌ Silent logic error |
| 5 | High | `health_service_events_subscribe` return value ignored | `main.c` | ⚠️ Silent failure |
| 6 | Medium | `window_destroy` on potentially stacked window in `shop_menu_show` | `shop_menu.c` | ⚠️ Latent crash |
| 7 | Medium | Services not unsubscribed in `deinit()` | `main.c` | ❌ Resource leak |
| 8 | Medium | Services subscribed in `init()` instead of `appear`/`disappear` | `main.c` | ❌ SDK anti-pattern |
| 9 | Medium | Struct reorder silently invalidates old saves | `game_state.h` | ❌ UX issue |
| 10 | Low | DOWN button registered with `up_click_handler` (naming) | `main.c` | — |
| 11 | Low | `click_count` as static local instead of module-level | `main.c` | — |
| 12 | Low | `format_mass` hardcodes buffer size 16 | `math_utils.c` | — |

---

## Recommended Fix Order

1. **Fix the SELECT click handler conflict (#1)** — this is a documented SDK violation and is the primary cause of the broken button behavior. Use `window_raw_click_subscribe` and a manual 5-second timer to separate tap and hold.

2. **Fix `game_state_buy_max()` to use closed-form calculation (#2)** — prevents hangs on shop long-press in mid/late game.

3. **Clamp the canvas ratio before casting to int (#3)** — one-line fix for undefined behavior.

4. **Handle `HealthEventSignificantUpdate` in `health_handler` (#4)** — fixes silent step loss across days.

5. **Check return value of `health_service_events_subscribe` (#5)** — adds a diagnostic log, prevents silent failure.

6. **Fix `shop_menu_show` to safely remove window before destroy (#6)** — hardens against future code changes.

7. **Move service subscriptions to `appear`/`disappear` (#8)** and **add unsubscriptions to `deinit` (#7)** — align with SDK lifecycle model.
