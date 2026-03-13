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

// Cette fonction lit le caractère UTF-8 et retourne l'index Unicode
// Elle avance aussi le pointeur de la chaîne pour passer les octets supplémentaires
uint32_t canvas_get_next_char(const char **text) {
  unsigned char c = (unsigned char)**text;
  
  if (c < 128) { // Caractère ASCII standard (1 octet)
    (*text)++;
    return c;
  } else if ((c & 0xE0) == 0xC0) { // Caractère accentué courant (2 octets)
    uint32_t res = ((c & 0x1F) << 6) | ((*(*text + 1)) & 0x3F);
    *text += 2;
    return res;
  }
  
  (*text)++; // Sécurité pour les caractères plus longs non gérés
  return '?';
}

uint32_t canvas_get_fallback_code(uint32_t code) {
  // Si le glyphe existe déjà dans notre police, on ne change rien
  if (bus_font[code].cols != NULL) return code;

  // Sinon, on cherche une alternative
  switch (code) {
    // Minuscules
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: return 'a'; // à á â ã ä
    case 0xE8: case 0xE9: case 0xEA: case 0xEB:           return 'e'; // è é ê ë
    case 0xEC: case 0xED: case 0xEE: case 0xEF:           return 'i'; // ì í î ï
    case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: return 'o'; // ò ó ô õ ö
    case 0xF9: case 0xFA: case 0xFB: case 0xFC:           return 'u'; // ù ú û ü
    case 0xE7:                                           return 'c'; // ç
    case 0xF1:                                           return 'n'; // ñ

    // Majuscules
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: return 'A'; // À Á Â Ã Ä
    case 0xC8: case 0xC9: case 0xCA: case 0xCB:           return 'E'; // È É Ê Ë
    case 0xCC: case 0xCD: case 0xCE: case 0xCF:           return 'I'; // Ì Í Î Ï
    case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: return 'O'; // Ò Ó Ô Õ Ö
    case 0xD9: case 0xDA: case 0xDB: case 0xDC:           return 'U'; // Ù Ú Û Ü
    case 0xC7:                                           return 'C'; // Ç
    case 0xD1:                                           return 'N'; // Ñ

    default: return code; // Pas de remplacement connu, on garde l'original
  }
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
  const char *ptr = text;
  int current_x = x_start;

  while (*ptr != '\0') {
    uint32_t code = canvas_get_fallback_code(canvas_get_next_char(&ptr)); // Récupère le code Unicode
    
    if (code < 256) { // On reste dans les limites de notre tableau
      canvas_draw_glyph(canvas, current_x, code, min_x, max_x);
      current_x += bus_font[code].width + 1; // +1 pour l'espacement entre lettres
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
