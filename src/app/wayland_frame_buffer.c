#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "wayland_frame_buffer.h"

// The compositor tells you "here's the format, width, height,
// and stride of the frame I'm about to give you."
static void frame_buffer(void *data, struct zwlr_screencopy_frame_v1 *frame,
                         u32 format, u32 width, u32 height, u32 stride) {
  Capture_Info *cap = data;
  cap->format = format;
  cap->width = width;
  cap->height = height;
  cap->stride = stride;
}

// The compositor says "I'm done telling you about the buffer
// format — now you can create a buffer and send it to me."
// then, we allocate this given buffer
static void frame_buffer_done(void *data,
                              struct zwlr_screencopy_frame_v1 *frame) {
  Capture_Info *cap = data;
  cap->buffer = create_shm_buffer(cap->shm, cap->width, cap->height,
                                  cap->stride, cap->format);
}

static void frame_ready(void *data, struct zwlr_screencopy_frame_v1 *frame,
                        u32 tv_sec_hi, u32 tv_sec_lo, u32 tv_nsec) {
  Capture_Info *cap = data;
  cap->ready = true;
  zwlr_screencopy_frame_v1_destroy(frame);
}

static void frame_failed(void *data, struct zwlr_screencopy_frame_v1 *frame) {
  Capture_Info *cap = data;
  cap->failed = true;
  zwlr_screencopy_frame_v1_destroy(frame);
}

static struct wl_buffer *create_shm_buffer(struct wl_shm *shm, u32 width,
                                           u32 height, u32 stride, u32 format) {
  i32 size = stride * height;

  i32 fd = syscall(SYS_memfd_create, "inkshot", 0);
  ftruncate(fd, size);

  void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
  struct wl_buffer *buffer =
      wl_shm_pool_create_buffer(pool, 0, width, height, stride, format);
  wl_shm_pool_destroy(pool);
  close(fd);

  return buffer;
}
