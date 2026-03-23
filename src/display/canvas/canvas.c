#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "canvas.h"
#include "../glyphs/font_manager.h"

canvas_t* canvas_create(uint32_t width, uint32_t height, uint32_t text_speed) {
  canvas_t* canvas = malloc(sizeof(canvas_t));
  uint32_t* table = calloc(width, sizeof(uint32_t));
  *canvas = (canvas_t){ table, width, height, text_speed, -width, 0, 0, 0 };
  return canvas;
}

void canvas_free(canvas_t *canvas) {
  free(canvas->canvas);
  free(canvas);
}

// Fonction pour dessiner un caractère dans le canvas
void canvas_draw_glyph(canvas_t* canvas, int x_pos, unsigned char c, int min_x, int max_x) {
  Glyph g = font_get_glyph(c);
  if (g.cols == NULL) return;

  int font_min_y = font_get_min_y();
  int font_max_y = font_get_max_y();
  int font_h = font_max_y - font_min_y + 1;
  int padding = (canvas->height - font_h) / 2;
  int shift = padding - font_min_y;

  for (int i = 0; i < g.width; i++) {
    int target_x = x_pos + i;
    // On ne dessine QUE si on est dans la zone autorisée
    if (target_x >= min_x && target_x < max_x) {
      uint32_t col = g.cols[i];
      if (shift > 0) col <<= shift;
      else if (shift < 0) col >>= -shift;
      canvas->canvas[target_x] |= col;
    }
  }
}

// Fonction pour dessiner une chaîne de caractères entière
int canvas_draw_string(canvas_t* canvas, message_manager* msgs, int x_start, const char *text, int min_x, int max_x) {
  const char *ptr = text;
  int current_x = x_start;

  while (*ptr != '\0') {
    if (*ptr == '\\') {
      if (*(ptr+1) == 'f') {
        char f = *(ptr+2);
        if (f == '0') font_set(msgs->default_font);
        else if (f == '1') font_set(&font_lp_6);
        else if (f == '2') font_set(&font_lp_a);
        ptr += 3;
        continue;
      } else if (*(ptr+1) == '\\') {
        ptr++; // On saute le premier \, on dessinera le deuxième
      }
    }

    uint32_t code = message_get_fallback_code(message_get_next_char(&ptr)); // Récupère le code Unicode
    
    if (code < 256) { // On reste dans les limites de notre tableau
      if (current_x > x_start) current_x += 1; // +1 pour l'espacement entre lettres
      canvas_draw_glyph(canvas, current_x, code, min_x, max_x);
      current_x += font_get_glyph(code).width;
    }
  }
  return current_x - x_start; // Retourne la largeur totale du texte dessiné
}

void canvas_loop(canvas_t *canvas, message_manager *msgs, void (*render)(canvas_t*)) {
  // 1. On vide le canvas et on récupère les infos du pipe
  int l = message_check_for_updates(msgs);
  memset(canvas->canvas, 0, canvas->width * sizeof(uint32_t));

  if(l) {
    canvas->tour = 0;
    canvas->timeElapsed = 0;
    canvas->offset = -canvas->width;
    message_update_dest_width(msgs);
  }

  // 2. On dessine le numéro fixe
  font_set(msgs->default_font);
  int debut_texte = 2 + canvas_draw_string(canvas, msgs, 2, message_get_numero(msgs), 2, canvas->width) + (message_get_numero(msgs)[0] != '\0' ? 1 : 0);

  // 3. On dessine une ligne de séparation verticale (x=18)
  // for (int y = 4; y < 20; y++) canvas[2 + debut_texte] |= (1 << y);
  // debut_texte += 4;

  int available_width = canvas->width - debut_texte; // Espace à droite de la barre

  // 4. On détermine la police à utiliser pour la destination
  const char* dest_text = message_get_dest(msgs);
  font_t font_to_use = message_get_best_font(msgs, dest_text, available_width, &canvas->dest_width);

  // 5. On dessine la destination
  if (canvas->dest_width <= available_width) {
    // --- CAS TEXTE COURT : FIXE ET CENTRÉ ---
    int padding = (available_width - canvas->dest_width) / 2;
    font_set(font_to_use);
    canvas_draw_string(canvas, msgs, debut_texte + padding, dest_text, debut_texte, canvas->width);

    // On remet l'offset à 0 car rien ne doit bouger
    canvas->offset = 0;

    // Gestion du temps
    if(++canvas->timeElapsed >= MAX_TEXT_DISPLAY / canvas->textSpeed) {
      canvas->timeElapsed = 0;
      canvas->tour++;
    }
  } else {
    // --- CAS TEXTE LONG : DÉFILEMENT ---
    font_set(font_to_use);
    canvas_draw_string(canvas, msgs, debut_texte - canvas->offset, dest_text, debut_texte, canvas->width);

    // Dessiner une deuxième fois le texte après pour un défilement infini fluide
    if ((msgs->size == 1 || canvas->tour + 1 < message_get_time(msgs)) && message_get_rebound(msgs) && debut_texte - canvas->offset + canvas->dest_width < (int)canvas->width) {
      font_set(font_to_use);
      canvas_draw_string(canvas, msgs, debut_texte - canvas->offset + canvas->dest_width + 20, dest_text, debut_texte, canvas->width);
    }

    // Gestion du défilement
    canvas->offset++;
    if (canvas->offset >= canvas->dest_width + 20) {
      canvas->offset = message_get_rebound(msgs) ? 0 : -canvas->width;

      // Gestion du temps
      canvas->tour++;
    }
  }

  // 7. Affichage
  render(canvas);

  // 8. Passage au texte suivant
  if(msgs->size > 1 && canvas->tour >= message_get_time(msgs)) {
    canvas->tour = 0;
    canvas->timeElapsed = 0;
    canvas->offset = -canvas->width;
    message_next(msgs);
  }

  usleep(canvas->textSpeed);
}
