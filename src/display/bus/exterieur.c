#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../render/render.h"

#define TEXT_SPEED 25000 // 25ms pour un défilement fluide (40 FPS)
#define PIPE_PATH "/tmp/bus_display_front_001"
#define MESSAGE_LIST_LENGTH 5
#define SCREEN_WIDTH 200
#define SCREEN_HEIGHT 24

void handle_sigint(int sig) {
  exit(0);
}

int main() {
  atexit(render_end);
  signal(SIGINT, handle_sigint);
  render_init();

  canvas_t* canvas = canvas_create(SCREEN_WIDTH, SCREEN_HEIGHT, TEXT_SPEED);
  message_manager* msgs = message_create_manager(MESSAGE_LIST_LENGTH, &canvas->dest_width, PIPE_PATH);
  
  message_add(msgs, "", "Ce bus ne prend pas de voyageurs", 1, 1);
  message_next(msgs);

  while (1) canvas_loop(canvas, msgs, render_show);

  return 0;
}