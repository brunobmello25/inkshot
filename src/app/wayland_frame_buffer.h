#ifndef WAYLAND_FRAME_BUFFER_H
#define WAYLAND_FRAME_BUFFER_H

#include "../base/base_core.h"
#include "generated/wlr-screen-copy.h"

typedef struct {
  struct wl_shm *shm;
  struct wl_buffer *buffer;
  void *data;
  u32 format, width, height, stride;
  b32 ready;
  b32 failed;
} Capture_Info;

static void frame_buffer(void *data, struct zwlr_screencopy_frame_v1 *frame,
                         u32 format, u32 width, u32 height, u32 stride);
static void frame_buffer_done(void *data,
                              struct zwlr_screencopy_frame_v1 *frame);
static void frame_ready(void *data, struct zwlr_screencopy_frame_v1 *frame,
                        u32 tv_sec_hi, u32 tv_sec_lo, u32 tv_nsec);
static void frame_failed(void *data, struct zwlr_screencopy_frame_v1 *frame);

static struct wl_buffer *create_shm_buffer(struct wl_shm *shm, u32 width,
                                           u32 height, u32 stride, u32 format);

#endif
