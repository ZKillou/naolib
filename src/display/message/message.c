#include "message.h"
#include <stdlib.h>
#include <string.h>
#include "../glyphs/font_manager.h"
#include "../glyphs/glyphs_lp_b.h"
#include "../glyphs/glyphs_lp_6.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

message_manager* message_create_manager(uint8_t total_size, int* dest_width, char* pipe_path) {
  message_manager* msgs = malloc(sizeof(message_manager));
  message_t* m = malloc(sizeof(message_t) * total_size);
  *msgs = (message_manager){ m, total_size, 0, 0, dest_width, pipe_path, &font_lp_b };
  return msgs;
}

uint32_t message_get_next_char(const char **text) {
  unsigned char c = (unsigned char)**text;
  if (c < 128) { (*text)++; return c; }
  else if ((c & 0xE0) == 0xC0) {
    uint32_t res = ((c & 0x1F) << 6) | ((*(*text + 1)) & 0x3F);
    *text += 2;
    return res;
  }
  (*text)++;
  return '?';
}

uint32_t message_get_fallback_code(uint32_t code) {
  if (font_get_glyph(code).cols != NULL) return code;
  switch (code) {
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: return 'a';
    case 0xE8: case 0xE9: case 0xEA: case 0xEB:           return 'e';
    case 0xEC: case 0xED: case 0xEE: case 0xEF:           return 'i';
    case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: return 'o';
    case 0xF9: case 0xFA: case 0xFB: case 0xFC:           return 'u';
    case 0xE7:                                           return 'c';
    case 0xF1:                                           return 'n';
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: return 'A';
    case 0xC8: case 0xC9: case 0xCA: case 0xCB:           return 'E';
    case 0xCC: case 0xCD: case 0xCE: case 0xCF:           return 'I';
    case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: return 'O';
    case 0xD9: case 0xDA: case 0xDB: case 0xDC:           return 'U';
    case 0xC7:                                           return 'C';
    case 0xD1:                                           return 'N';
    default: return code;
  }
}

void message_update_dest_width(message_manager* msgs) {
  *(msgs->dest_width) = 0;
  const char* ptr = msgs->messages[msgs->current].message;
  font_set(msgs->default_font); // On commence toujours avec la font par défaut

  while (*ptr != '\0') {
    if (*ptr == '\\') {
      if (*(ptr+1) == 'f') {
        char f = *(ptr+2);
        if (f == '0') font_set(&font_lp_b);
        else if (f == '1') font_set(&font_lp_6);
        ptr += 3;
        continue;
      } else if (*(ptr+1) == '\\') {
        ptr++;
      }
    }

    uint32_t code = message_get_fallback_code(message_get_next_char(&ptr));
    if (code < 256) {
      if (*(msgs->dest_width) > 0) *(msgs->dest_width) += 1;
      *(msgs->dest_width) += font_get_glyph(code).width;
    }
  }
}

message_t* message_next(message_manager* msgs) {
  if(!msgs->size) {
    *(msgs->dest_width) = 0;
    return &default_message;
  }
  msgs->current = (msgs->current + 1) % msgs->size;
  message_update_dest_width(msgs);
  return &(msgs->messages[msgs->current]);
}

int message_add(message_manager* msgs, char* numero, char* message, uint8_t rebound, uint8_t time) {
  if(msgs->size + 1 == msgs->total_size) return 0;
  msgs->messages[msgs->size] = (message_t){ strdup(numero), strdup(message), rebound, time };
  msgs->size++;
  return 1;
}

int message_update(message_manager* msgs, int idx, char* num, char* msg, int reb, int t) {
  if (idx < 0 || idx >= msgs->total_size) return 0;
  if (idx >= msgs->size) return message_add(msgs, num, msg, reb, t);
  
  // Libération de l'ancienne mémoire si elle existe
  if (msgs->messages[idx].numero) free(msgs->messages[idx].numero);
  if (msgs->messages[idx].message) free(msgs->messages[idx].message);

  // Allocation et copie
  msgs->messages[idx].numero = strdup(num);
  msgs->messages[idx].message = strdup(msg);
  msgs->messages[idx].rebound = (uint8_t)reb;
  msgs->messages[idx].time = (uint8_t)t;

  return 1;
}

int message_clear(message_manager* msgs) {
  for(int i=0; i<msgs->size; i++) {
    free(msgs->messages[i].numero); free(msgs->messages[i].message);
  }
  msgs->size = 0; msgs->current = 0;
  return 1;
}

void message_free_manager(message_manager* msgs) {
  message_clear(msgs);
  free(msgs->messages);
  free(msgs);
}

char* message_get_numero(message_manager* msgs) {
  return (msgs->size ? msgs->messages[msgs->current] : default_message).numero;
}

char* message_get_dest(message_manager* msgs) {
  return (msgs->size ? msgs->messages[msgs->current] : default_message).message;
}

uint8_t message_get_rebound(message_manager* msgs) {
  return (msgs->size ? msgs->messages[msgs->current] : default_message).rebound;
}

uint8_t message_get_time(message_manager* msgs) {
  return (msgs->size ? msgs->messages[msgs->current] : default_message).time;
}

int message_parse_pipe_command(message_manager* msgs, char* cmd) {
  char *token = strtok(cmd, "|");
  if (!token) return 0;

  if (strcmp(token, "SET") == 0 || strcmp(token, "ADD") == 0) {
    int idx = (strcmp(token, "ADD") == 0) ? msgs->size : atoi(strtok(NULL, "|"));
    char *num = strtok(NULL, "|");
    if(strcmp(num, " ") == 0) num = "";
    char *msg = strtok(NULL, "|");
    int reb = atoi(strtok(NULL, "|"));
    int t = atoi(strtok(NULL, "|"));
    int reset = atoi(strtok(NULL, "|"));

    if (msg) {
      int res = 0;
      if(strcmp(token, "ADD") == 0) res = message_add(msgs, num, msg, reb, t);
      else res = message_update(msgs, idx, num, msg, reb, t);
      
      if (idx == msgs->current) message_update_dest_width(msgs);
      return reset ? res : 0;
    }
  } else if (strcmp(token, "CLR") == 0) {
    return message_clear(msgs);
  } else if (strcmp(token, "FNT") == 0) {
    char *token = strtok(NULL, "|");
    if (!token) return 0;
    int f = atoi(token);
    if (f == 0) msgs->default_font = &font_lp_b;
    else if (f == 1) msgs->default_font = &font_lp_6;
    message_update_dest_width(msgs);
    return 1;
  }

  return 0;
}

int message_check_for_updates(message_manager* msgs) {
  if(!msgs->pipe_path) return 0;

  static int fd = -1;
  if (fd == -1) {
    mkfifo(msgs->pipe_path, 0666);
    fd = open(msgs->pipe_path, O_RDONLY | O_NONBLOCK);
  }

  char buffer[512];
  ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
  if (n <= 0) return 0;

  buffer[n] = '\0';
  return message_parse_pipe_command(msgs, buffer);
}