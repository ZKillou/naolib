#pragma once

#include <stdint.h>

typedef struct { const uint32_t *cols; uint8_t width; int8_t y_offset; } Glyph;

typedef struct {
  const Glyph *glyphs;
  uint8_t min_y;
  uint8_t max_y;
} Font;