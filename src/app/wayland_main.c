#include <stdio.h>
#include <string.h>
#include <wayland-client-core.h>

#include "wayland_frame_buffer.h"
#include "wayland_main.h"

//- headers
#include "../base/base_core.h"
#include "generated/wlr-layer-shell.h"
#include "generated/wlr-screen-copy.h"
#include "generated/xdg-shell-client-protocol.h"

// - source files
#include "../base/base_arena.c"
#include "../os/os_core_linux.c"
#include "generated/wlr-layer-shell.c"
#include "generated/wlr-screen-copy.c"
#include "generated/xdg-shell-client-protocol.c"
#include "wayland_frame_buffer.c"

static void output_geometry(void *data, struct wl_output *wl_output, i32 x,
                            i32 y, i32 phys_w, i32 phys_h, i32 subpixel,
                            const char *make, const char *model,
                            i32 transform) {
  Output_Info *out = data;
  out->x = x;
  out->y = y;
}

static void output_mode(void *data, struct wl_output *wl_output, u32 flags,
                        i32 width, i32 height, i32 refresh) {
  Output_Info *out = data;
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    out->width = width;
    out->height = height;
  }
}

static void output_done(void *data, struct wl_output *wl_output) {
  Output_Info *out = data;
  out->done = true;
}

static void output_scale(void *data, struct wl_output *wl_output, i32 factor) {
  Output_Info *out = data;
  out->scale = factor;
}

static void output_name(void *data, struct wl_output *wl_output,
                        const char *name) {
  Output_Info *out = data;
  snprintf(out->name, sizeof(out->name), "%s", name);
}

static void output_description(void *data, struct wl_output *wl_output,
                               const char *description) {
  (void)data;
  (void)wl_output;
  (void)description;
}

static const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
    .done = output_done,
    .scale = output_scale,
    .name = output_name,
    .description = output_description,
};

static void registry_global(void *data, struct wl_registry *registry, u32 name,
                            const char *interface, u32 version) {
  State *state = data;

  if (strcmp(interface, wl_output_interface.name) == 0) {
    if (state->output_count >= MAX_OUTPUTS) {
      fprintf(stderr, "too many outputs, skipping\n");
      return;
    }

    Output_Info *out = &state->outputs[state->output_count++];
    out->scale = 1;
    out->wl_output = wl_registry_bind(registry, name, &wl_output_interface,
                                      version < 4 ? version : 4);
    wl_output_add_listener(out->wl_output, &output_listener, out);
  }

  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compositor = wl_registry_bind(
        registry, name, &wl_compositor_interface, version < 4 ? version : 4);
  }

  if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    state->wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface,
                                      version < 4 ? version : 4);
  }

  if (strcmp(interface, wl_shm_interface.name) == 0) {
    state->shm = wl_registry_bind(registry, name, &wl_shm_interface,
                                  version < 4 ? version : 4);
  }

  if (strcmp(interface, zwlr_screencopy_manager_v1_interface.name) == 0) {
    state->screencopy_manager =
        wl_registry_bind(registry, name, &zwlr_screencopy_manager_v1_interface,
                         version < 4 ? version : 4);
  }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
                                   u32 name) {
  (void)data;
  (void)registry;
  (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

int main(void) {
  State state = {0};

  state.display = wl_display_connect(NULL);
  if (!state.display) {
    fprintf(stderr, "failed to connect to wayland display\n");
    return 1;
  }

  state.registry = wl_display_get_registry(state.display);
  wl_registry_add_listener(state.registry, &registry_listener, &state);

  // first roundtrip: discover globals and bind outputs
  wl_display_roundtrip(state.display);
  // second roundtrip: receive output events (geometry, mode, done)
  wl_display_roundtrip(state.display);

  // after setup roundtrips, capture each output
  Capture_Info captures[MAX_OUTPUTS] = {0};
  struct zwlr_screencopy_frame_v1_listener frame_listener = {
      .buffer = frame_buffer,
      .buffer_done = frame_buffer_done,
      .ready = frame_ready,
      .failed = frame_failed,
  };

  for (i32 output_index = 0; output_index < state.output_count;
       output_index += 1) {
    captures[output_index].shm = state.shm;
    struct zwlr_screencopy_frame_v1 *frame =
        zwlr_screencopy_manager_v1_capture_output(
            state.screencopy_manager, 0, state.outputs[output_index].wl_output);
    zwlr_screencopy_frame_v1_add_listener(frame, &frame_listener,
                                          &captures[output_index]);
  }
  wl_display_roundtrip(state.display);

  printf("found %d output(s):\n\n", state.output_count);
  for (i32 i = 0; i < state.output_count; i++) {
    Output_Info *out = &state.outputs[i];
    printf("  %s\n", out->name[0] ? out->name : "(unnamed)");
    printf("    position  : %d,%d\n", out->x, out->y);
    printf("    resolution: %dx%d\n", out->width, out->height);
    printf("    scale     : %d\n", out->scale);
    printf("    compositor: %p\n", (void *)state.compositor);
    printf("    wm_base   : %p\n", (void *)state.wm_base);
    printf("    wl_shm    : %p\n", (void *)state.shm);
    printf("\n");

    printf("    %s: capture: %s (%dx%d)\n", state.outputs[i].name,
           captures[i].ready ? "ok" : "FAILED", captures[i].width,
           captures[i].height);
  }

  zwlr_screencopy_manager_v1_destroy(state.screencopy_manager);
  for (i32 i = 0; i < state.output_count; i++) {
    wl_output_destroy(state.outputs[i].wl_output);
  }
  wl_registry_destroy(state.registry);
  wl_display_disconnect(state.display);

  return 0;
}
