#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

// Inclusion des données générées par le script Python
#include "glyphs_ext.h"

#define REBOUND 1
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 24
#define ORANGE "\033[38;5;214m"
#define RESET  "\033[0m"

// Le buffer mémoire (notre "carte graphique")
uint32_t canvas[SCREEN_WIDTH];

// Restauration du terminal à la sortie
void restore_terminal() {
  printf("\033[?25h" RESET); // Réaffiche le curseur et reset les couleurs
  printf("\nGirouette éteinte.\n");
  fflush(stdout);
}

void handle_sigint(int sig) {
  exit(0);
}

// Fonction pour dessiner un caractère dans le canvas
void draw_glyph(int x_pos, unsigned char c, int min_x, int max_x) {
  Glyph g = bus_font[c];
  if (g.cols == NULL) return;

  for (int i = 0; i < g.width; i++) {
    int target_x = x_pos + i;
    // On ne dessine QUE si on est dans la zone autorisée
    if (target_x >= min_x && target_x < max_x) {
        canvas[target_x] |= g.cols[i];
    }
  }
}

// Fonction pour dessiner une chaîne de caractères entière
int draw_string(int x_start, const char *text, int min_x, int max_x) {
  int current_x = x_start;
  for (int i = 0; text[i] != '\0'; i++) {
    unsigned char c = (unsigned char)text[i];
    draw_glyph(current_x, c, min_x, max_x);
    current_x += bus_font[c].width + 1; // +1 pour l'espacement entre lettres
  }
  return current_x - x_start; // Retourne la largeur totale du texte dessiné
}

// MOTEUR DE RENDU : Affiche le canvas dans le terminal (1 ligne de terminal = 1 pixel)
void render_to_terminal() {
  // \033[H ramène le curseur en haut à gauche sans effacer (plus fluide que \033[J)
  printf("\033[H");

  printf(" ╔");
  for (int i = 0; i < SCREEN_WIDTH + 2; i++) printf("═");
  printf("╗\n");

  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    printf(" ║ "); // Marge à gauche
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      // On vérifie si le bit 'y' de la colonne 'x' est à 1
      if ((canvas[x] >> y) & 1) {
        printf(ORANGE "█" RESET);
      } else {
        printf(" ");
      }
    }
    printf(" ║\n");
  }

  printf(" ╚");
  for (int i = 0; i < SCREEN_WIDTH + 2; i++) printf("═");
  printf("╝\n");

  fflush(stdout);
}

int main() {
  // Initialisation
  atexit(restore_terminal);
  signal(SIGINT, handle_sigint);
  printf("\033[?25l\033[2J"); // Cache le curseur et efface l'écran une fois

  const char *numero = "E5";
  const char *destination = "CARQUEFOU";
  
  int offset = 0;
  
  // Calcul de la largeur de la destination pour savoir quand reboucler
  // (On simule un dessin dans le vide pour avoir la largeur)
  int dest_width = 0;
  for(int i=0; destination[i]; i++) dest_width += bus_font[(unsigned char)destination[i]].width + (destination[i] != '\0');

  while (1) {
    // 1. On vide le canvas
    memset(canvas, 0, sizeof(canvas));

    // 2. On dessine le numéro fixe
    int debut_texte = 2 + draw_string(2, numero, 2, SCREEN_WIDTH);

    // 3. On dessine une ligne de séparation verticale (x=18)
    // for (int y = 4; y < 20; y++) canvas[2 + debut_texte] |= (1 << y);
    // debut_texte += 4;

    int available_width = SCREEN_WIDTH - debut_texte; // Espace à droite de la barre

    // 4. On dessine la destination (débute après la barre à x=22)
    if (dest_width <= available_width) {
      // --- CAS TEXTE COURT : FIXE ET CENTRÉ ---
      // On calcule l'espace vide pour centrer le texte dans les 140px restants
      int padding = (available_width - dest_width) / 2;
      draw_string(debut_texte + padding, destination, debut_texte, SCREEN_WIDTH);
      
      // 5. On remet l'offset à 0 car rien ne doit bouger
      offset = 0; 
    } else {
      // --- CAS TEXTE LONG : DÉFILEMENT ---
      // L'offset crée le mouvement
      draw_string(debut_texte - offset, destination, debut_texte, SCREEN_WIDTH);
      
      // Dessiner une deuxième fois le texte après pour un défilement infini fluide
      if (REBOUND && debut_texte - offset + dest_width < SCREEN_WIDTH) {
        draw_string(debut_texte - offset + dest_width + 20, destination, debut_texte, SCREEN_WIDTH);
      }

      // 5. Gestion du défilement
      offset++;
      if (offset >= dest_width + 20) {
        offset = 0;
      }
    }
  
    // 6. Affichage
    render_to_terminal();
    usleep(25000); // 50ms pour un défilement fluide (20 FPS)
  }

  return 0;
}