# inkshot — Session Context

Cite this file at the start of a new session to resume where we left off.
**The assistant must update this file at the END of every session** (status, learnings,
next step). Last updated: 2026-08-24

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

Primary target is user's **work laptop running GNOME Wayland (GNOME 42.9)**.

Why not layer-shell: Mutter has never implemented `wlr-layer-shell` and refuses to
(gnome-shell#1141). Verified July 2026: still the holdout. Layer-shell overlay is
impossible on GNOME; may be added later as optional enhancement for Hyprland/sway.

Why not XWayland/X11: under Wayland, X11 clients cannot read native window pixels nor
stack above them — dead end (that's why Flameshot breaks on Wayland).

### Pipeline

1. **Capture**: D-Bus call to `org.freedesktop.portal.Screenshot` → compositor writes
   PNG to disk, returns URI → load pixels into memory.
   - Open question: on GNOME 42 portal may prompt; fallback = private
     `org.gnome.Shell.Screenshot` D-Bus API (test when we get there).
2. **Windows**: one `wl_surface` per output → `xdg_wm_base_get_xdg_surface` →
   `xdg_surface_get_toplevel` → `xdg_toplevel_set_fullscreen(output)`.
   Borderless, covers exactly that monitor. One window per display.
3. **Render**: PNG pixels into memfd/mmap buffer → `wl_shm` pool/buffer → attach+commit.
   Later: upload as GLES texture via EGL for hardware-accelerated drawing tools.
4. **Selection**: drag rectangle across displays. Pointer events are surface-local;
   convert with `global = output.x + local_x`. Selection rect lives in global State
   (NOT per-window — pointer leave/enter mid-drag must not reset it). Bright inside,
   dimmed outside; each window renders its slice.
5. **Annotate**: lines/arrows/rectangles on frozen image (GLES once EGL lands).
6. **Finish**: Enter → crop region → PNG → clipboard.

### Multi-display coordinates

- `xdg-output-manager` protocol required: mutter reported all outputs at 0,0 via plain
  `wl_output.geometry`; xdg-output gives true logical positions (must verify).
- Mixed scales (eDP-1 scale=2): mind `wl_surface.set_buffer_scale`, physical vs logical.

## Environment Facts

- This machine: Ubuntu, GNOME Shell 42.9, currently in **X11 session** — must log into
  "Ubuntu on Wayland" session to test anything.
- Installed: wayland-scanner ✓, libwayland-dev ✓ (wayland-client.pc), EGL/GLES ✓.
  Missing: libcairo2-dev, wlr-protocols package. wayland-protocols v1.25 (old but has
  stable xdg-shell XML at `/usr/share/wayland-protocols/stable/xdg-shell/`).
- Mutter 49+ supports `ext-image-copy-capture-v1` (direct capture, no portal) — this
  machine's GNOME 42 does not. Revisit if laptop upgrades past GNOME 49.

## Tech Stack

- C (not Rust), GCC, unity builds, arenas
- `libwayland-client` directly (no toolkit); xdg-shell now, EGL/GLES later
- D-Bus for portal (GIO available via pkg-config if needed)
- xkbcommon for keyboard (Escape/Enter)
- Docs: `docs/wayland-area-selection.md` (research; NOTE: its layer-shell
  recommendation is superseded by this strategy), `docs/wayland-book/` submodule

## Roadmap

1. ~~Display enumeration~~ DONE (predates CONTEXT.md; code in `src/app/wayland_main.c`)
2. Borderless fullscreen `xdg_toplevel` per display + solid-color shm render
3. Real capture: portal screenshot → render image in windows
4. Rectangular selection across displays (global coords, dim outside)
5. Annotation tools (lines/arrows/rects) — decide EGL vs software here
6. Clipboard copy of final composited image on Enter

## Current Status

Step 2 not started. Plan agreed:
- A: generate xdg-shell bindings (wayland-scanner client-header + private-code),
  extend build.sh; bind `wl_compositor`, `xdg_wm_base`, `wl_shm` (+ xdg-output-manager)
- B: replace roundtrips with dispatch loop + SIGINT exit flag (watch EINTR)
- C: configure dance: commit bare surface → wait configure → ack_configure → THEN
  attach buffer. Also ping→pong or compositor kills us. Fullscreen(toplevel, output).
- D: solid color per display (distinct colors) via memfd→shm pool→XRGB8888 buffer
- E: verify coverage/no decorations/clean teardown

## Gotchas & Decisions Log

- Configure-before-attach ordering is mandatory (protocol error otherwise).
- First commit must be empty (no buffer) to trigger initial configure.
- Selection state global, never per-window (mid-drag pointer handoff between windows).
- All-zeros output positions from wl_output on GNOME → use xdg-output-manager.
- Portal XMLs: xdg-shell from system wayland-protocols; nothing needs vendoring yet
  (no layer-shell/screencopy in this architecture).
