#pragma once

#include <stdint.h>
#include "../message/message.h"

#define MAX_TEXT_DISPLAY 6000000 // Le texte fixe reste affiché 6s

typedef struct canvas_t {
  uint32_t* canvas;
  uint32_t width;
  uint32_t height;
  uint32_t textSpeed;
  int offset;
  int tour;
  int timeElapsed;
  int dest_width;
} canvas_t;

canvas_t* canvas_create(uint32_t width, uint32_t height, uint32_t text_speed);
void canvas_free(canvas_t* canvas);
void canvas_draw_glyph(canvas_t* canvas, int x_pos, unsigned char c, int min_x, int max_x);
int canvas_draw_string(canvas_t* canvas, message_manager* msgs, int x_start, const char *text, int min_x, int max_x);
void canvas_loop(canvas_t* canvas, message_manager* msgs, void (*render)(canvas_t*));