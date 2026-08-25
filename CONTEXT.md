# inkshot — Session Context

Cite this file at the start of a new session to resume where we left off.
**The assistant must update this file at the END of every session** (status, learnings,
next step). Last updated: 2026-08-25 (session 3)

---

## What This Is

Wayland-native Flameshot replacement, written in C. Freeze-and-select: capture screen →
show frozen image fullscreen → select region → annotate → clipboard.

## Working Rules (MUST follow)

1. **NEVER write code for the user.** Guide only — explain concepts, point to protocols,
   describe approaches, review their code when asked. User writes all code.
2. **Unity build system.** All `.c` files `#include`d into one translation unit (see
   `src/app/wayland_main.c`). New `.c` files get included the same way.
3. **Arena allocators for everything** (`src/base/base_arena.h`: `ArenaAlloc`,
   `PushArray`, `PushStruct`, `TempBegin/End`). No malloc/free for app data.
   Exception: `wl_shm` buffers must use `memfd_create` + `mmap` (protocol requirement).
4. Style: PascalCase types (`Output_Info`), snake_case fields, minimal comments.
5. Build: `bin/build.sh` (GCC + pkg-config), output `data/inkshot`.
6. Be terse in answers — user dislikes verbosity.

## Architecture Strategy

**Dual codepath: wlr-screencopy for capture, xdg-shell for overlay windows.**

Primary target: user's **work laptop running GNOME Wayland (GNOME 42.9)**.
Also tested on: Fedora 42 desktop (wlroots-based compositor).

Capture uses `wlr-screencopy` protocol (widely supported, deprecated but functional).
Overlay uses `xdg_shell` — one `xdg_toplevel` per output, fullscreened.

Why not layer-shell for overlay: Mutter never implemented `wlr-layer-shell`. Layer-shell
may be added later as optional enhancement for Hyprland/sway users.

### Pipeline

1. **Capture**: `zwlr_screencopy_manager_v1_capture_output` per output → wl_shm buffer
   with pixels. Frame listener: buffer → buffer_done → copy → ready/failed.
2. **Windows**: one `wl_surface` per output → `xdg_surface` → `xdg_toplevel` →
   `xdg_toplevel_set_fullscreen(output)`. Borderless fullscreen per display.
3. **Render**: attach captured pixels via wl_shm pool/buffer → attach+commit.
   Later: GLES texture via EGL for hw-accelerated annotation tools.
4. **Selection**: drag rect across displays; `global = output.x + local_x`. Selection
   state lives in global State (pointer leave/enter mid-drag must not reset it).
   Bright inside, dimmed outside; each window renders its slice.
5. **Annotate**: lines/arrows/rects on frozen image.
6. **Finish**: Enter → crop → PNG → clipboard.

### Multi-display coordinates

- Session observed so far reports REAL positions via plain `wl_output` (see Learnings),
  but bind `xdg-output-manager` anyway when selection work starts (names + guaranteed
  logical layout).

## Environment Facts

- **Fedora 42** (desktop): primary dev machine, wlroots compositor. Packages:
  `wlr-protocols-devel` for protocol XML (vendored in `protocols/`).
- **Ubuntu 22.04** (work laptop): GNOME Shell 42.9, **default session is X11** — testing
  requires logging into a Wayland session ("Ubuntu on Wayland").
- Installed (Ubuntu): wayland-scanner ✓, libwayland-dev ✓, EGL/GLES ✓. Missing: libcairo2-dev.
  wayland-protocols v1.25 (stable xdg-shell XML at
  `/usr/share/wayland-protocols/stable/xdg-shell/`).
- Mutter 49+ supports `ext-image-copy-capture-v1` (direct capture, no portal) — GNOME
  42 does not. Revisit only if laptop upgrades past GNOME 49.

## Tech Stack

- C, GCC, unity builds, arenas
- libwayland-client directly; xdg-shell + wlr-screencopy now, EGL/GLES later; xkbcommon for keys
- Generated bindings live in `src/app/generated/` (wlr-screen-copy.{h,c},
  wlr-layer-shell.{h,c}, xdg-shell-client-protocol.{h,c}),
  produced by build.sh via wayland-scanner from vendored XMLs in `protocols/`.
  Include order in unity TU:
  base_core.h → generated .h files → base_arena.c → os_core_linux.c → generated .c files → app code.

## Project Layout

- `src/app/wayland_main.c` — entry point (unity TU root)
- `src/app/wayland_main.h` — `State`, `Output_Info`, `MAX_OUTPUTS`
- `src/app/wayland_frame_buffer.h` — `Capture_Info` + forward declarations
- `src/app/wayland_frame_buffer.c` — screencopy listener callbacks + `create_shm_buffer`
- `src/app/generated/` — wayland-scanner output (headers + implementation per protocol)
- `src/base/base_core.h` — type aliases (u8/u16/.../i32/f32 etc), macros
- `src/base/base_arena.{h,c}` — arena allocator
- `src/os/os_core_linux.c` — OS abstraction (mmap/mprotect wrappers)
- `bin/build.sh` — build script (wayland-scanner + GCC)
- `protocols/` — vendored protocol XML files (distro-agnostic):
  `wlr-screencopy-unstable-v1.xml`, `wlr-layer-shell-unstable-v1.xml`, `xdg-shell.xml`
- `docs/` — research docs + wayland-book submodule
- `data/` — build output (gitignored)

## Roadmap

1. ~~Display enumeration~~ DONE
2. ~~Screencopy~~ IN PROGRESS (code written, crashes before main — debugging pending)
3. Fullscreen xdg-shell windows + event loop
4. Render captured pixels into windows
5. Rectangular cross-display selection (global coords, dim outside)
6. Annotation tools (lines/arrows/rects) — EGL vs software decision here
7. Clipboard copy of final image on Enter

## Current Status

### Step 2 — Screencopy: Code Written, Crash on Startup

**What's implemented:**
- Protocol XMLs vendored in `protocols/` (screencopy + layer-shell + xdg-shell)
- Generated bindings in `src/app/generated/` (wayland-scanner calls correct: client-header + private-code)
- `State.screencopy_manager` field bound in `registry_global`
- `Capture_Info` struct in `wayland_frame_buffer.h` — holds shm pointer, buffer, pixel data, format/dimensions, ready/failed flags
- Frame listener callbacks in `wayland_frame_buffer.c`:
  - `frame_buffer` — saves format/width/height/stride from compositor
  - `frame_buffer_done` — creates shm buffer (missing: `zwlr_screencopy_frame_v1_copy` call)
  - `frame_ready` — sets `ready = true`, destroys frame
  - `frame_failed` — sets `failed = true`, destroys frame
- `create_shm_buffer` — uses `syscall(SYS_memfd_create)` + `mmap` + `wl_shm_create_pool` + `wl_shm_pool_create_buffer`
- Capture loop after two roundtrips, third roundtrip to dispatch frame events
- Cleanup: destroys screencopy manager before output/registry/display

**Known bugs to fix:**
1. App crashes before reaching `main()` — cause unknown, possibly generated code issue or protocol version mismatch
2. `frame_buffer_done` is missing `zwlr_screencopy_frame_v1_copy(frame, cap->buffer)` — compositor never writes pixels
3. `cap->data` (mmap pointer) is never stored — pixels can't be read later

## Gotchas & Decisions Log

- Configure-before-attach ordering mandatory (protocol error otherwise); first commit
  empty to trigger initial configure.
- Selection state global, never per-window.
- Bind version caps: `min(advertised, N)` — binding above advertised = disconnect.
  (wm_base/shm capped 4 in code today — harmless, tighten to 1 whenever touching it.)
- `struct wl_shm` not `xdg_wm_shm`; assign binds directly to State pointer fields
  (no copy-through-NULL deref).
- `wl_shm` buffers MUST use `memfd_create` + `mmap` — arena allocator can't satisfy
  the fd requirement of `wl_shm_create_pool`.
- `memfd_create` requires `_GNU_SOURCE` or use `syscall(SYS_memfd_create, ...)` directly
  (codebase uses syscall variant to avoid macro pollution).
- Editor task-runner wraps program output and errors with Rust-style JSON parse noise —
  ignore; verify programs from a plain shell.

## Learnings Log

1. This Wayland session reports real output positions: `(2744,0)` 1920x1080,
   `(1920,1080)` 3440x1440, `(0,1320)` 1920x1200 — coherent desk layout. Old
   "all zeros" note came from a different compositor/session.
2. Same session: scale=1 everywhere (laptop panel was scale 2 in step 1's run) and all
   outputs unnamed → NOT the same environment as step 1. Identify which WM/compositor
   this session runs (ask user next time; check $XDG_CURRENT_DESKTOP / $DESKTOP_SESSION).
3. Portal XMLs: nothing needs vendoring under this architecture (no wlr protocols used for portal path).
4. wlr-protocols not packaged on Ubuntu 22.04 — must vendor XML files. Fedora has `wlr-protocols-devel`.
5. `wayland-scanner client-header` for `.c` files is wrong — must use `private-code`. Client-header generates header content (include guards, declarations), not implementation.
