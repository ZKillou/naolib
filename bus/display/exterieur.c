#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>

// Structures
#include "msg_ext.h"
// Inclusion des données générées par le script Python
#include "glyphs_ext.h"

#define TEXT_SPEED 25000 // 25ms pour un défilement fluide (40 FPS)
#define MAX_TEXT_DISPLAY 6000000 // Le texte fixe reste affiché 6s
#define PIPE_PATH "/tmp/bus_display_front_001"
#define MESSAGE_LIST_LENGTH 5
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 24
#define ORANGE "\033[38;5;214m"
#define RESET  "\033[0m"

// Le buffer mémoire (notre "carte graphique")
uint32_t canvas[SCREEN_WIDTH];

// Messages
MessageExterieur default_message = (MessageExterieur){ "", "", 0, 0 };
MessageExterieur* messages;
int current_message = 0;
int messages_size = 0;

// Restauration du terminal à la sortie
void restore_terminal() {
  printf("\033[?25h" RESET); // Réaffiche le curseur et reset les couleurs
  printf("\nGirouette éteinte.\n");
  fflush(stdout);
}

void handle_sigint(int sig) {
  exit(0);
}

// Gestion des messages
void update_dest_width(int* dest_width) {
  *dest_width = 0;
  for(int i=0; messages[current_message].message[i]; i++) *dest_width += bus_font[(unsigned char)messages[current_message].message[i]].width + (messages[current_message].message[i] != '\0');
}

MessageExterieur* next_message(int* dest_width) {
  if(!messages_size) {
    *dest_width = 0;
    return &default_message;
  }
  current_message = (current_message + 1) % messages_size;
  update_dest_width(dest_width);
  return &messages[current_message];
}


int add_message(char* numero, char* message, uint8_t rebound, uint8_t time) {
  if(messages_size + 1 == MESSAGE_LIST_LENGTH) return 0;
  messages[messages_size] = (MessageExterieur){ strdup(numero), strdup(message), rebound, time };
  messages_size++;
  return 1;
}

int update_message(int idx, char* num, char* msg, int reb, int t) {
  if (idx < 0 || idx >= MESSAGE_LIST_LENGTH) return 0;
  if (idx >= messages_size) return add_message(num, msg, reb, t);
  
  // Libération de l'ancienne mémoire si elle existe
  if (messages[idx].numero) free(messages[idx].numero);
  if (messages[idx].message) free(messages[idx].message);

  // Allocation et copie
  messages[idx].numero = strdup(num);
  messages[idx].message = strdup(msg);
  messages[idx].rebound = (uint8_t)reb;
  messages[idx].time = (uint8_t)t;

  return 1;
}

int clear_messages() {
  for(int i=0; i<messages_size; i++) {
    free(messages[i].numero); free(messages[i].message);
  }
  messages_size = 0; current_message = 0;
  return 1;
}

char* get_numero() {
  return (messages_size ? messages[current_message] : default_message).numero;
}

char* get_dest() {
  return (messages_size ? messages[current_message] : default_message).message;
}

uint8_t get_rebound() {
  return (messages_size ? messages[current_message] : default_message).rebound;
}

uint8_t get_time() {
  return (messages_size ? messages[current_message] : default_message).time;
}

// Pipe
int parse_pipe_command(char* cmd) {
  char *token = strtok(cmd, "|");
  if (!token) return 0;

  if (strcmp(token, "SET") == 0 || strcmp(token, "ADD") == 0) {
    int idx = (strcmp(token, "ADD") == 0) ? messages_size : atoi(strtok(NULL, "|"));
    char *num = strtok(NULL, "|");
    if(strcmp(num, " ") == 0) num = "";
    char *msg = strtok(NULL, "|");
    int reb = atoi(strtok(NULL, "|"));
    int t = atoi(strtok(NULL, "|"));

    //printf("i%d : %s -> %s (reb %d, t %d)\n", idx, num, msg, reb, t);
    if (msg) {
      if(strcmp(token, "ADD") == 0) return add_message(num, msg, reb, t);
      else return update_message(idx, num, msg, reb, t);
    }
  } else if (strcmp(token, "CLR") == 0) {
    return clear_messages();
  }

  return 0;
}

int check_for_updates() {
  static int fd = -1;
  if (fd == -1) {
    mkfifo(PIPE_PATH, 0666);
    fd = open(PIPE_PATH, O_RDONLY | O_NONBLOCK);
  }

  char buffer[512];
  ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
  if (n <= 0) return 0;

  buffer[n] = '\0';
  return parse_pipe_command(buffer);
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
  
  int offset = -SCREEN_WIDTH;
  int tour = 0;
  int timeElapsed = 0;
  
  // Calcul de la largeur de la destination pour savoir quand reboucler
  int dest_width = 0;
  messages = malloc(sizeof(MessageExterieur) * MESSAGE_LIST_LENGTH);
  add_message("", "Ce bus ne prend pas de voyageurs", 1, 1);
  next_message(&dest_width);

  while (1) {
    // 1. On vide le canvas et on récupère les infos du pipe
    int l = check_for_updates();
    memset(canvas, 0, sizeof(canvas));

    if(l) {
      tour = 0;
      timeElapsed = 0;
      offset = -SCREEN_WIDTH;
      update_dest_width(&dest_width);
    }

    // 2. On dessine le numéro fixe
    int debut_texte = 2 + draw_string(2, get_numero(), 2, SCREEN_WIDTH);

    // 3. On dessine une ligne de séparation verticale (x=18)
    // for (int y = 4; y < 20; y++) canvas[2 + debut_texte] |= (1 << y);
    // debut_texte += 4;

    int available_width = SCREEN_WIDTH - debut_texte; // Espace à droite de la barre

    // 4. On dessine la destination (débute après la barre à x=22)
    if (dest_width <= available_width) {
      // --- CAS TEXTE COURT : FIXE ET CENTRÉ ---
      // On calcule l'espace vide pour centrer le texte dans les 140px restants
      int padding = (available_width - dest_width) / 2;
      draw_string(debut_texte + padding, get_dest(), debut_texte, SCREEN_WIDTH);
      
      // 5. On remet l'offset à 0 car rien ne doit bouger
      offset = 0;

      // 6. Gestion du temps
      if(++timeElapsed >= MAX_TEXT_DISPLAY / TEXT_SPEED) {
        timeElapsed = 0;
        tour++;
      }
    } else {
      // --- CAS TEXTE LONG : DÉFILEMENT ---
      // L'offset crée le mouvement
      draw_string(debut_texte - offset, get_dest(), debut_texte, SCREEN_WIDTH);
      
      // Dessiner une deuxième fois le texte après pour un défilement infini fluide
      if ((messages_size == 1 || tour + 1 < get_time()) && get_rebound() && debut_texte - offset + dest_width < SCREEN_WIDTH) {
        draw_string(debut_texte - offset + dest_width + 20, get_dest(), debut_texte, SCREEN_WIDTH);
      }

      // 5. Gestion du défilement
      offset++;
      if (offset >= dest_width + 20) {
        offset = get_rebound() ? 0 : -SCREEN_WIDTH;

        // 6. Gestion du temps
        tour++;
      }
    }

    // 7. Affichage
    render_to_terminal();
    
    // 8. Passage au texte suivant
    if(messages_size > 1 && tour >= get_time()) {
      tour = 0;
      timeElapsed = 0;
      offset = -SCREEN_WIDTH;
      next_message(&dest_width);
    }

    usleep(TEXT_SPEED);
  }

  return 0;
}