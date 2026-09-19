#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct Window
{
  void*    window;
  void*    layer;
  uint32_t width;
  uint32_t height;
} Window;

bool
window_open(Window* window, uint32_t width, uint32_t height, const char* title);

void
window_close(Window* window);

bool
window_poll(Window* window);

void
window_show_image(Window* window, void* image);
