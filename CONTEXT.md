# inkshot — Session Context

Cite this file at the start of a new session to resume where we left off.
**The assistant must update this file at the END of every session** (status, learnings,
next step). Last updated: 2026-08-24 (session 2)

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
4. Style: PascalCase types (`Output_Info`), snake_case fields, minimal comments.
5. Build: `bin/build.sh` (GCC + pkg-config), output `data/inkshot`.
6. Be terse in answers — user dislikes verbosity.

## Architecture Strategy (decided 2026-08-24)

**Single universal codepath via plain `xdg_shell` — NOT wlr-layer-shell.**

Primary target: user's **work laptop running GNOME Wayland (GNOME 42.9)**.

Why not layer-shell: Mutter never implemented `wlr-layer-shell`, refuses to
(gnome-shell#1141, re-verified July 2026). Why not XWayland/X11: under Wayland, X11
clients can't read native pixels nor stack above them (that's why Flameshot breaks).
Layer-shell may be added later as optional enhancement for Hyprland/sway users.

### Pipeline

1. **Capture**: D-Bus `org.freedesktop.portal.Screenshot` → PNG file URI → load pixels.
   Open question: GNOME 42 portal may prompt; fallback = private
   `org.gnome.Shell.Screenshot` D-Bus API.
2. **Windows**: one `wl_surface` per output → `xdg_surface` → `xdg_toplevel` →
   `xdg_toplevel_set_fullscreen(output)`. Borderless fullscreen per display.
3. **Render**: pixels into memfd/mmap → `wl_shm` pool/buffer → attach+commit.
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

- Ubuntu, GNOME Shell 42.9 installed; **default session is X11** — testing requires
  logging into a Wayland session ("Ubuntu on Wayland").
- Installed: wayland-scanner ✓, libwayland-dev ✓, EGL/GLES ✓. Missing: libcairo2-dev.
  wayland-protocols v1.25 (stable xdg-shell XML at
  `/usr/share/wayland-protocols/stable/xdg-shell/`).
- Mutter 49+ supports `ext-image-copy-capture-v1` (direct capture, no portal) — GNOME
  42 does not. Revisit only if laptop upgrades past GNOME 49.
- Unknown: which compositor the current Wayland session actually is (see Learnings #3).

## Tech Stack

- C, GCC, unity builds, arenas
- libwayland-client directly; xdg-shell now, EGL/GLES later; xkbcommon for keys
- Generated bindings live in `src/os/generated/` (xdg-shell-client-protocol.{h,c}),
  produced by build.sh via wayland-scanner. Include order in unity TU:
  base_core.h → xdg .h → base_arena.c → xdg .c → os_core_linux.c → app code.

## Roadmap

1. ~~Display enumeration~~ DONE
2. Borderless fullscreen windows + event loop ← **IN PROGRESS**
3. Portal screenshot → render image in windows
4. Rectangular cross-display selection (global coords, dim outside)
5. Annotation tools (lines/arrows/rects) — EGL vs software decision here
6. Clipboard copy of final image on Enter

## Current Status

### Step 2 — Phase A COMPLETE (session 2)

- build.sh runs wayland-scanner (client-header + private-code) before gcc
- Bound globals: `wl_compositor` (cap 4), `xdg_wm_base`, `wl_shm` — stored in State,
  verified non-NULL in Wayland session
- registry_global() structure settled: MAX_OUTPUTS guard inside wl_output branch;
  other globals assign handle straight into State fields, no listeners

### NEXT: Step 2 — Phase B/C/D

- **B**: replace two-roundtrip pattern with `while (running) wl_display_dispatch()`;
  SIGINT handler flips flag; watch EINTR on interrupted dispatch. Keep startup prints.
- **C**: per-output window: create surface → get_xdg_surface → get_toplevel →
  set_fullscreen(toplevel, output). Configure dance: commit bare surface FIRST → wait
  configure → ack_configure → THEN attach buffer. Add wm_base ping→pong listener
  (mandatory). Store handles in Output_Info.
- **D**: solid distinct color per display: memfd → ftruncate → mmap → wl_shm pool →
  XRGB8888 buffer → attach/damage/commit. Mind buffer_scale vs logical size if any
  display reports scale > 1.
- Verify: every monitor covered borderless by its color; clean teardown on Ctrl+C.

## Gotchas & Decisions Log

- Configure-before-attach ordering mandatory (protocol error otherwise); first commit
  empty to trigger initial configure.
- Selection state global, never per-window.
- Bind version caps: `min(advertised, N)` — binding above advertised = disconnect.
  (wm_base/shm capped 4 in code today — harmless, tighten to 1 whenever touching it.)
- `struct wl_shm` not `xdg_wm_shm`; assign binds directly to State pointer fields
  (no copy-through-NULL deref).
- Editor task-runner wraps program output and errors with Rust-style JSON parse noise —
  ignore; verify programs from a plain shell.

## Learnings Log

1. This Wayland session reports real output positions: `(2744,0)` 1920x1080,
   `(1920,1080)` 3440x1440, `(0,1320)` 1920x1200 — coherent desk layout. Old
   "all zeros" note came from a different compositor/session.
2. Same session: scale=1 everywhere (laptop panel was scale 2 in step 1's run) and all
   outputs unnamed → NOT the same environment as step 1. Identify which WM/compositor
   this session runs (ask user next time; check $XDG_CURRENT_DESKTOP / $DESKTOP_SESSION).
3. Portal XMLs: nothing needs vendoring under this architecture (no wlr protocols used).
