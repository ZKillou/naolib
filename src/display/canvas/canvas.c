#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "canvas.h"
#include "../glyphs/glyphs_lp_b.h"

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
  Glyph g = bus_font[c];
  if (g.cols == NULL) return;

  for (int i = 0; i < g.width; i++) {
    int target_x = x_pos + i;
    // On ne dessine QUE si on est dans la zone autorisée
    if (target_x >= min_x && target_x < max_x) {
      canvas->canvas[target_x] |= g.cols[i];
    }
  }
}

// Fonction pour dessiner une chaîne de caractères entière
int canvas_draw_string(canvas_t* canvas, int x_start, const char *text, int min_x, int max_x) {
  int current_x = x_start;
  for (int i = 0; text[i] != '\0'; i++) {
    unsigned char c = (unsigned char)text[i];
    canvas_draw_glyph(canvas, current_x, c, min_x, max_x);
    current_x += bus_font[c].width + 1; // +1 pour l'espacement entre lettres
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
  int debut_texte = 2 + canvas_draw_string(canvas, 2, message_get_numero(msgs), 2, canvas->width);

  // 3. On dessine une ligne de séparation verticale (x=18)
  // for (int y = 4; y < 20; y++) canvas[2 + debut_texte] |= (1 << y);
  // debut_texte += 4;

  int available_width = canvas->width - debut_texte; // Espace à droite de la barre

  // 4. On dessine la destination (débute après la barre à x=22)
  if (canvas->dest_width <= available_width) {
    // --- CAS TEXTE COURT : FIXE ET CENTRÉ ---
    // On calcule l'espace vide pour centrer le texte dans les 140px restants
    int padding = (available_width - canvas->dest_width) / 2;
    canvas_draw_string(canvas, debut_texte + padding, message_get_dest(msgs), debut_texte, canvas->width);
    
    // 5. On remet l'offset à 0 car rien ne doit bouger
    canvas->offset = 0;

    // 6. Gestion du temps
    if(++canvas->timeElapsed >= MAX_TEXT_DISPLAY / canvas->textSpeed) {
      canvas->timeElapsed = 0;
      canvas->tour++;
    }
  } else {
    // --- CAS TEXTE LONG : DÉFILEMENT ---
    // L'offset crée le mouvement
    canvas_draw_string(canvas, debut_texte - canvas->offset, message_get_dest(msgs), debut_texte, canvas->width);
    
    // Dessiner une deuxième fois le texte après pour un défilement infini fluide
    if ((msgs->size == 1 || canvas->tour + 1 < message_get_time(msgs)) && message_get_rebound(msgs) && debut_texte - canvas->offset + canvas->dest_width < (int)canvas->width) {
      canvas_draw_string(canvas, debut_texte - canvas->offset + canvas->dest_width + 20, message_get_dest(msgs), debut_texte, canvas->width);
    }

    // 5. Gestion du défilement
    canvas->offset++;
    if (canvas->offset >= canvas->dest_width + 20) {
      canvas->offset = message_get_rebound(msgs) ? 0 : -canvas->width;

      // 6. Gestion du temps
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
