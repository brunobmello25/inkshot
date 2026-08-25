#ifndef WAYLAND_MAIN_H
#define WAYLAND_MAIN_H

#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "../base/base_core.h"

#define MAX_OUTPUTS 8

typedef struct {
  struct wl_output *wl_output;
  i32 x, y;
  i32 width, height;
  i32 scale;
  char name[64];
  b32 done;
} Output_Info;

typedef struct {
  struct wl_display *display;
  struct wl_registry *registry;
  Output_Info outputs[MAX_OUTPUTS];
  i32 output_count;
  struct wl_compositor *compositor;
  struct xdg_wm_base *wm_base;
  struct wl_shm *shm;
  struct zwlr_screencopy_manager_v1 *screencopy_manager;
} State;

#endif
