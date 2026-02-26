#include <stdint.h>

#ifndef MSG_EXT_H
#define MSG_EXT_H

typedef struct MessageExterieur {
  char* numero;
  char* message;
  uint8_t rebound;
  uint8_t time;
} MessageExterieur;

#endif