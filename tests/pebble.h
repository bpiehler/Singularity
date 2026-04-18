#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Mock Pebble types for host-side testing
typedef struct {
  int tm_sec;
} tm;

typedef enum {
  APP_LOG_LEVEL_INFO
} AppLogLevel;

#define APP_LOG(level, fmt, ...) 
#define PBL_IF_ROUND_ELSE(round, rect) (rect)

static inline void persist_write_data(uint32_t key, void *data, size_t size) {}
static inline bool persist_exists(uint32_t key) { return false; }
static inline void persist_read_data(uint32_t key, void *data, size_t size) {}
