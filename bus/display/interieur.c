#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define WIDTH 20  // Largeur de l'écran de la girouette
#define DELAY 150000 // Vitesse (0.15 seconde)

typedef struct Message {
  char* contenu;
  int taille;
} Message;

// Fonction pour restaurer le curseur
void restore_cursor() {
    printf("\033[?25h"); // Code ANSI pour réafficher le curseur
    printf("\nGirouette arrêtée.\n");
    fflush(stdout);
}

// Gestionnaire de signal pour Ctrl+C
void handle_sigint(int sig) {
    exit(0); // Appelle automatiquement les fonctions enregistrées avec atexit
}

void print_girouette(char *line1, char *line2, int offset) {
    int len1 = strlen(line1);
    int len2 = strlen(line2);

    // Retour au début du terminal
    printf("\033[H\033[J"); 

    printf("╔══════════════════════╗\n");
    printf("║ ");
    
    // Ligne 1 : Défilement
    for (int i = 0; i < WIDTH; i++) {
        putchar(line1[(offset + i) % len1]);
    }
    
    printf(" ║\n║ ");

    // Ligne 2 : Défilement (peut être synchro ou fixe)
    for (int i = 0; i < WIDTH; i++) {
        putchar(line2[(offset + i) % len2]);
    }

    printf(" ║\n");
    printf("╚══════════════════════╝\n");
    fflush(stdout); // Force l'affichage immédiat
}

int main() {
    // 1. Enregistrer la fonction de nettoyage
    atexit(restore_cursor);

    // 2. Capturer le signal Ctrl+C (SIGINT)
    signal(SIGINT, handle_sigint);

    // On ajoute des espaces à la fin pour l'effet de vide entre deux passages
    char *text1 = " 92 GARE MONTPARNASSE     ";
    char *text2 = " VIA PORTE D'ORLEANS      ";
    int offset = 0;

    // Masquer le curseur pour l'esthétique
    printf("\033[?25l");

    while (1) {
        print_girouette(text1, text2, offset);
        offset++;
        usleep(DELAY);
    }

    return 0;
}