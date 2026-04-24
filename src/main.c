#include <pebble.h>
#include <math.h>
#include "game_state.h"
#include "math_utils.h"
#include "shop_menu.h"
#include "stats_menu.h"

static Window *s_main_window;
static TextLayer *s_mass_layer, *s_gravity_layer;
static Layer *s_canvas_layer;
static GameState s_state __attribute__((aligned(8))); 

static void update_display();

#if defined(PBL_HEALTH)
static int s_last_step_count = 0;
static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventSignificantUpdate) {
    s_last_step_count = 0;
  }

  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(HealthMetricStepCount, start, end);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    int total_steps = (int)health_service_sum_today(HealthMetricStepCount);
    if (s_last_step_count == 0) {
      // Baseline initialization:
      // We already credited all historical steps up to 'now' in game_state_apply_offline_gains().
      // This baseline prevents double-counting the current day's steps.
      s_last_step_count = total_steps;
    } else {
      int delta = total_steps - s_last_step_count;
      if (delta > 0) {
        game_state_add_steps(&s_state, delta);
      }
      s_last_step_count = total_steps;
    }
  }
}
#endif

static AppTimer *s_tap_timer = NULL;
static int s_hold_time_ms = 0;
static bool s_is_app_exiting = false;
static int s_taps_since_last_tick = 0;
static bool s_app_has_focus = true;
static bool s_is_collapsing = false;
static bool s_is_flashing = false;
static int s_collapse_frame = 0;

static void collapse_timer_callback(void *data) {
  if (s_is_app_exiting) return;
  s_collapse_frame++;
  if (s_collapse_frame < 40) {
    layer_mark_dirty(s_canvas_layer);
    app_timer_register(30, collapse_timer_callback, NULL);
  } else if (s_collapse_frame == 40) {
    s_is_flashing = true;
    layer_mark_dirty(s_canvas_layer);
    app_timer_register(150, collapse_timer_callback, NULL);
  } else {
    game_state_prestige(&s_state);
    s_is_collapsing = false;
    s_is_flashing = false;
    s_collapse_frame = 0;
    update_display();
  }
}

void main_trigger_big_bang() {
  s_is_collapsing = true;
  s_is_flashing = false;
  s_collapse_frame = 0;
  app_timer_register(30, collapse_timer_callback, NULL);
}

static void focus_handler(bool in_focus) {
  s_app_has_focus = in_focus;
  if (s_app_has_focus && !s_is_collapsing) {
    game_state_apply_offline_gains(&s_state);
    update_display();
  }
}

static void tap_timer_callback(void *data) {
  if (s_is_app_exiting || !s_app_has_focus || s_is_collapsing) {
    s_tap_timer = NULL;
    return;
  }
  
  // Auto-tap logic: Add mass and schedule next tap
  game_state_add_mass(&s_state, game_state_calculate_tap_strength(&s_state));
  s_taps_since_last_tick++;
  game_state_update_cache(&s_state);
  
  s_tap_timer = app_timer_register(200, tap_timer_callback, NULL);
}

static void select_down_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_is_app_exiting || s_is_collapsing) return;
  if (s_tap_timer) app_timer_cancel(s_tap_timer);
  
  // 1. Immediate Single Tap
  game_state_add_mass(&s_state, game_state_calculate_tap_strength(&s_state));
  s_taps_since_last_tick++;
  game_state_update_cache(&s_state);
  
  // 2. Schedule start of auto-tap after 500ms hold
  s_tap_timer = app_timer_register(500, tap_timer_callback, NULL);
}

static void select_up_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_tap_timer) { 
    app_timer_cancel(s_tap_timer); 
    s_tap_timer = NULL; 
  }
}

typedef struct {
  GColor core;
  GColor aura;
} EraColors;

static EraColors get_era_colors(int era) {
  #if defined(PBL_BW)
  return (EraColors){ .core = GColorWhite, .aura = GColorWhite };
  #else
  switch (era) {
    case 0: return (EraColors){ .core = GColorWhite, .aura = GColorLightGray };
    case 1: return (EraColors){ .core = GColorIslamicGreen, .aura = GColorMalachite };
    case 2: return (EraColors){ .core = GColorCyan, .aura = GColorElectricBlue };
    case 3: return (EraColors){ .core = GColorYellow, .aura = GColorOrange };
    case 4: return (EraColors){ .core = GColorVividViolet, .aura = GColorShockingPink };
    default: return (EraColors){ .core = GColorWhite, .aura = GColorWhite };
  }
  #endif
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  if (s_is_flashing) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    return;
  }
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 35), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, bounds.size.h - 25, bounds.size.w, 25), 0, GCornerNone);
  int well_y_center = 35 + ((bounds.size.h - 35 - 25) / 2);
  GPoint center = GPoint(bounds.size.w / 2, well_y_center);

  int radius = 10;
  if (s_is_collapsing) {
    double start_log = log10(s_state.mass > 1.0 ? s_state.mass : 1.0);
    if (start_log > 36.0) start_log = 36.0;
    int start_radius = 10 + (int)((start_log / 36.0) * 50.0);
    radius = start_radius - (int)((float)s_collapse_frame / 40.0f * (float)start_radius);
    if (radius < 0) radius = 0;
  } else {
    double log_mass = log10(s_state.mass > 1.0 ? s_state.mass : 1.0);
    if (log_mass > 36.0) log_mass = 36.0;
    radius = 10 + (int)((log_mass / 36.0) * 50.0);
  }

  bool is_prestige_ready = (s_state.mass >= PRESTIGE_THRESHOLD);
  bool is_unstable = (s_state.mass >= PRESTIGE_THRESHOLD * 0.9 && !s_is_collapsing);
  if (is_unstable) {
    center.x += (rand() % 3) - 1;
    center.y += (rand() % 3) - 1;
  }

  int era = game_state_get_era(&s_state);
  EraColors colors = get_era_colors(era);

  if (is_unstable || s_is_collapsing) {
    #if defined(PBL_COLOR)
    // Slow color cycle at 1Hz for prestige warning
    time_t now = time(NULL);
    switch (now % 4) {
      case 0: colors.core = GColorRed; colors.aura = GColorDarkCandyAppleRed; break;
      case 1: colors.core = GColorOrange; colors.aura = GColorRed; break;
      case 2: colors.core = GColorYellow; colors.aura = GColorOrange; break;
      case 3: colors.core = GColorWhite; colors.aura = GColorYellow; break;
    }
    #else
    colors.core = GColorWhite; colors.aura = GColorWhite;
    #endif
  }

  if (radius > 0) {
    // Draw Aura (Accretion Disk)
    graphics_context_set_fill_color(ctx, colors.aura);
    graphics_fill_circle(ctx, center, radius);

    // Draw Core (Matter)
    // Core is 80% of aura size, but at least 2px smaller
    int core_radius = (radius * 8) / 10;
    if (core_radius > radius - 2) core_radius = radius - 2;
    if (core_radius < 1) core_radius = 1;

    graphics_context_set_fill_color(ctx, colors.core);
    graphics_fill_circle(ctx, center, core_radius);

    // Draw Contrasting Rim
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, (is_unstable || s_is_collapsing) ? GColorWhite : colors.aura);
    graphics_draw_circle(ctx, center, radius + 1);
  }

  #if defined(PBL_BW)
  if (!is_unstable && !s_is_collapsing && radius > 4) {
    if (era == 1) {
      for (int i = -radius; i < radius; i += 4) {
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_draw_line(ctx, GPoint(center.x - radius, center.y + i), GPoint(center.x + radius, center.y + i));
      }
    } else if (era == 2) {
      graphics_context_set_stroke_color(ctx, GColorBlack);
      graphics_draw_circle(ctx, center, radius / 2);
    } else if (era == 3) {
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_fill_circle(ctx, center, radius / 2);
    }
  }
  #endif

  if (!s_is_collapsing) {
    // --- Navigation & Status Indicators ---
    // Use relative padding for round screens (approx 15% of width)
    int margin_h = PBL_IF_ROUND_ELSE(bounds.size.w * 15 / 100, 10);
    int indicator_x = bounds.size.w - margin_h;
    GFont font_icons = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    
    // 1. UP: Cosmic Ledger (Hint)
    int up_y = PBL_IF_ROUND_ELSE(42, 38);
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, "i", font_icons, GRect(indicator_x, up_y, 10, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

    // 2. SELECT: Cosmic Forge / Status
    if (is_prestige_ready) {
      graphics_context_set_text_color(ctx, GColorRed);
      graphics_draw_text(ctx, "!", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
                         GRect(indicator_x - 2, center.y - 12, 12, 25), 
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    } else {
      if (s_state.upgrade_ready) {
        graphics_context_set_text_color(ctx, colors.core);
        graphics_draw_text(ctx, ">", font_icons, 
                           GRect(indicator_x - 3, center.y - 10, 10, 20), 
                           GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      } else {
        graphics_context_set_text_color(ctx, GColorDarkGray);
        graphics_draw_text(ctx, "o", font_icons, 
                           GRect(indicator_x - 3, center.y - 10, 10, 20), 
                           GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      }
    }

    // 3. DOWN: Amass / Tap (Hint)
    int down_y = PBL_IF_ROUND_ELSE(bounds.size.h - 58, bounds.size.h - 45);
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, "+", font_icons, GRect(indicator_x - 3, down_y, 10, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void update_display() {
  if (!s_mass_layer || !s_gravity_layer) return;
  static char s_mass_buffer[64];
  static char s_gravity_buffer[64];
  char val_buffer[32];
  format_mass(s_state.mass, val_buffer);
  snprintf(s_mass_buffer, sizeof(s_mass_buffer), "%s", val_buffer);
  text_layer_set_text(s_mass_layer, s_mass_buffer);
  format_mass(game_state_calculate_gravity(&s_state), val_buffer);
  snprintf(s_gravity_buffer, sizeof(s_gravity_buffer), "G: %s/s", val_buffer);
  text_layer_set_text(s_gravity_layer, s_gravity_buffer);
  if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (s_is_collapsing) return;

  // Power Save Mode: Check battery
  BatteryChargeState battery = battery_state_service_peek();
  bool power_save = (battery.charge_percent <= 20 && !battery.is_charging);
  
  // In Power Save, only update full logic every 4 seconds to save CPU/Screen
  if (power_save && (tick_time->tm_sec % 4 != 0)) return;

  int seconds_elapsed = power_save ? 4 : 1;
  s_taps_since_last_tick = 0;
  
  double gravity = game_state_calculate_gravity(&s_state);
  game_state_add_mass(&s_state, gravity * (double)seconds_elapsed);
  s_state.total_playtime_seconds += seconds_elapsed;
  
  game_state_update_cache(&s_state); // Updates Peak Mass and Upgrade Flags
  update_display();
  
  // Consolidate saving: Save every 5 minutes (at :00 seconds)
  if (tick_time->tm_sec == 0 && tick_time->tm_min % 5 == 0) {
    game_state_save(&s_state);
  }
}

static void open_shop_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_is_collapsing) return;
  shop_menu_show(&s_state, update_display);
}

static void open_stats_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_is_collapsing) return;
  stats_menu_show(&s_state);
}

static void click_config_provider(void *context) {
  // DOWN for Tapping and Auto-tap (Unified to resolve conflict)
  window_raw_click_subscribe(BUTTON_ID_DOWN, select_down_handler, select_up_handler, NULL);
  
  // SELECT for Shop
  window_single_click_subscribe(BUTTON_ID_SELECT, open_shop_handler);
  
  // UP for Stats
  window_single_click_subscribe(BUTTON_ID_UP, open_stats_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  // Add vertical margins for round screens to prevent horizontal cutoff
  int margin_v = PBL_IF_ROUND_ELSE(10, 0);
  
  s_mass_layer = text_layer_create(GRect(0, margin_v, bounds.size.w, 35));
  text_layer_set_background_color(s_mass_layer, GColorClear);
  text_layer_set_text_color(s_mass_layer, GColorWhite);
  text_layer_set_text_alignment(s_mass_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mass_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mass_layer));
  
  s_gravity_layer = text_layer_create(GRect(0, bounds.size.h - 25 - margin_v, bounds.size.w, 25));
  text_layer_set_background_color(s_gravity_layer, GColorClear);
  text_layer_set_text_color(s_gravity_layer, GColorCeleste);
  text_layer_set_text_alignment(s_gravity_layer, GTextAlignmentCenter);
  text_layer_set_font(s_gravity_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_gravity_layer));
  update_display();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_mass_layer);
  text_layer_destroy(s_gravity_layer);
  layer_destroy(s_canvas_layer);
  s_mass_layer = NULL; s_gravity_layer = NULL; s_canvas_layer = NULL;
}

static void init() {
  s_is_app_exiting = false;
  srand(time(NULL)); // Seed RNG for visual effects
  
  if (!game_state_load(&s_state)) game_state_init(&s_state);
  else game_state_apply_offline_gains(&s_state);
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) { .load = main_window_load, .unload = main_window_unload });
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  app_focus_service_subscribe(focus_handler);
  
  #if defined(PBL_HEALTH)
  if (!health_service_events_subscribe(health_handler, NULL)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Health subscription failed!");
  }
  #endif
}

static void deinit() {
  s_is_app_exiting = true;
  tick_timer_service_unsubscribe();
  app_focus_service_unsubscribe();
  #if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
  #endif
  game_state_save(&s_state);
  shop_menu_deinit();
  stats_menu_deinit();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
