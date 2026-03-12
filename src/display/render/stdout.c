#include "render.h"
#include <stdio.h>

#define ORANGE "\033[38;5;214m"
#define RESET  "\033[0m"

// Cache le curseur et efface l'écran une fois
void render_init() {
  printf("\033[?25l\033[2J");
}

// MOTEUR DE RENDU : Affiche le canvas dans le terminal (1 ligne de terminal = 1 pixel)
void render_show(canvas_t* canvas) {
  // \033[H ramène le curseur en haut à gauche sans effacer (plus fluide que \033[J)
  printf("\033[H");

  printf(" ╔");
  for (int i = 0; i < canvas->width + 2; i++) printf("═");
  printf("╗\n");

  for (int y = 0; y < canvas->height; y++) {
    printf(" ║ "); // Marge à gauche
    for (int x = 0; x < canvas->width; x++) {
      // On vérifie si le bit 'y' de la colonne 'x' est à 1
      if ((canvas->canvas[x] >> y) & 1) {
        printf(ORANGE "█" RESET);
      } else {
        printf(" ");
      }
    }
    printf(" ║\n");
  }

  printf(" ╚");
  for (int i = 0; i < canvas->width + 2; i++) printf("═");
  printf("╝\n");

  fflush(stdout);
}

// Restauration du terminal à la sortie
void render_end() {
  printf("\033[?25h" RESET); // Réaffiche le curseur et reset les couleurs
  printf("\nGirouette éteinte.\n");
  fflush(stdout);
}