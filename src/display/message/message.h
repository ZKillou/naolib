#pragma once

#include <stdint.h>

typedef struct message_t {
  char* numero;
  char* message;
  uint8_t rebound;
  uint8_t time;
} message_t;

static message_t default_message = (message_t){ "", "", 0, 0 };

typedef struct message_manager {
  message_t* messages;
  uint8_t total_size;
  uint8_t size;
  uint8_t current;
  int* dest_width;
  char* pipe_path;
} message_manager;

// Méthodes de manipulation des messages
message_manager* message_create_manager(uint8_t total_size, int* dest_width, char* pipe_path);
void message_free_manager(message_manager* msgs);
void message_update_dest_width(message_manager* msgs);
message_t* message_next(message_manager* msgs);
int message_add(message_manager* msgs, char* numero, char* message, uint8_t rebound, uint8_t time);
int message_update(message_manager* msgs, int idx, char* num, char* msg, int reb, int t);
int message_clear(message_manager* msgs);
char* message_get_numero(message_manager* msgs);
char* message_get_dest(message_manager* msgs);
uint8_t message_get_rebound(message_manager* msgs);
uint8_t message_get_time(message_manager* msgs);

// Méthodes de lecture des messages
uint32_t message_get_next_char(const char **text);
uint32_t message_get_fallback_code(uint32_t code);

// Écoute des messages
int message_parse_pipe_command(message_manager* msgs, char* cmd);
int message_check_for_updates(message_manager* msgs);