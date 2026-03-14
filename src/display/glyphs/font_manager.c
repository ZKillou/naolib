#include "font_manager.h"
#include <stddef.h>

static font_t current_font = NULL;

void font_set(font_t font) {
  current_font = font;
}

font_t font_get_current(void) {
  return current_font;
}

Glyph font_get_glyph(unsigned char c) {
  if (current_font == NULL) return (Glyph){NULL, 0, 0};
  return current_font->glyphs[c];
}

int font_get_min_y(void) {
  if (current_font == NULL) return 32;
  return current_font->min_y;
}

int font_get_max_y(void) {
  if (current_font == NULL) return -1;
  return current_font->max_y;
}
