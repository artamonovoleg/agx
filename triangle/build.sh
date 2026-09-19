#!/bin/sh
set -e
cd "$(dirname "$0")"
clang -fno-objc-arc -O2 -w -isystem ../ -I . \
  main.m window.m \
  ../agx/agx.c \
  ../agx/agx_driver_programs.c \
  ../agx/agx_end_of_tile_program.c \
  ../agx/agx_refusal.c \
  ../agx/agx_shader.c \
  ../agx/agx_swapchain.m \
  -framework IOKit -framework CoreFoundation -framework Foundation \
  -framework Cocoa -framework QuartzCore -framework IOSurface \
  -o triangle
