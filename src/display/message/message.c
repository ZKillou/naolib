#include "message.h"
#include <stdlib.h>
#include <string.h>
#include "../glyphs/glyphs_lp_b.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

message_manager* message_create_manager(uint8_t total_size, int* dest_width, char* pipe_path) {
  message_manager* msgs = malloc(sizeof(message_manager));
  message_t* m = malloc(sizeof(message_t) * total_size);
  *msgs = (message_manager){ m, total_size, 0, 0, dest_width, pipe_path };
  return msgs;
}

void message_update_dest_width(message_manager* msgs) {
  *(msgs->dest_width) = 0;
  for(int i=0; msgs->messages[msgs->current].message[i]; i++) *(msgs->dest_width) += bus_font[(unsigned char)msgs->messages[msgs->current].message[i]].width + (msgs->messages[msgs->current].message[i] != '\0');
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

    if (msg) {
      if(strcmp(token, "ADD") == 0) return message_add(msgs, num, msg, reb, t);
      else return message_update(msgs, idx, num, msg, reb, t);
    }
  } else if (strcmp(token, "CLR") == 0) {
    return message_clear(msgs);
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