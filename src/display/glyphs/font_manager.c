#include "font_manager.h"
#include <stddef.h>

static font_t current_font = NULL;
static int current_min_y = 32;
static int current_max_y = -1;

void font_set(font_t font) {
    current_font = font;
    current_min_y = 32;
    current_max_y = -1;

    if (font == NULL) return;

    for (int i = 0; i < 256; i++) {
        Glyph g = (*font)[i];
        if (g.cols == NULL) continue;
        for (int j = 0; j < g.width; j++) {
            uint32_t col = g.cols[j];
            if (col == 0) continue;
            for (int k = 0; k < 32; k++) {
                if ((col >> k) & 1) {
                    if (k < current_min_y) current_min_y = k;
                    if (k > current_max_y) current_max_y = k;
                }
            }
        }
    }
}

font_t font_get_current(void) {
    return current_font;
}

Glyph font_get_glyph(unsigned char c) {
    if (current_font == NULL) return (Glyph){NULL, 0};
    return (*current_font)[c];
}

int font_get_min_y(void) {
    return current_min_y;
}

int font_get_max_y(void) {
    return current_max_y;
}
