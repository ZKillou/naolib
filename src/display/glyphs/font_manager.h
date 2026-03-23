#pragma once

#include "glyph.h"
#include "glyphs_lp_b.h"
#include "glyphs_lp_6.h"
#include "glyphs_lp_a.h"

// Définition d'un type pour une police
typedef const Font* font_t;

/**
 * Définit la police d'écriture courante.
 * @param font Pointeur vers la structure Font de la police.
 */
void font_set(font_t font);

/**
 * Récupère la police d'écriture courante.
 * @return Pointeur vers le tableau de 256 Glyphs de la police.
 */
font_t font_get_current(void);

/**
 * Récupère un glyphe de la police courante.
 * @param c Caractère ASCII/Code.
 * @return Le glyphe correspondant.
 */
Glyph font_get_glyph(unsigned char c);

/**
 * Récupère la position du premier bit allumé de la police courante.
 */
int font_get_min_y(void);

/**
 * Récupère la position du dernier bit allumé de la police courante.
 */
int font_get_max_y(void);