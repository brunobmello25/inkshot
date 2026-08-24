# inkshot — Session Context

Cite this file at the start of a new session to resume where we left off.
**The assistant must update this file at the END of every session** (status, learnings,
next step). Last updated: 2026-08-24

---

## What This Is

Wayland-native Flameshot replacement, written in C. Freeze-and-select architecture:
capture all outputs → overlay frozen captures via layer-shell → user selects region →
annotate → copy to clipboard.

## Working Rules (MUST follow)

1. **NEVER write code for the user.** Guide only — explain concepts, point to protocols/
   docs, describe approaches, review their code when asked. The user writes all code.
2. **Unity build system.** All `.c` files are `#include`d into one translation unit
   (see `src/app/wayland_main.c` including `base_arena.c`, `os_core_linux.c`). New `.c`
   files get included the same way, not compiled separately.
3. **Arena allocators for everything.** No malloc/free scattered around. Use the Arena
   API from `src/base/base_arena.h` (`ArenaAlloc`, `PushArray`, `PushStruct`,
   `TempBegin`/`TempEnd`, ...). Wayland/shell resources that need explicit destroy calls
   are fine, but any data allocation goes through arenas.
4. Code style: PascalCase types/functions (`Output_Info`, `State`), lowercase
   snake_case fields, no comments unless necessary.
5. Build with `bin/build.sh` (GCC), output to `data/inkshot` currently — eventually
   `target/inkshot`.

## Tech Stack

- C (not Rust), GCC via `bin/build.sh`
- `libwayland-client` directly (like slurp/grim) — no toolkit
- Cairo planned for rendering; xkbcommon for keyboard
- Protocol bindings generated with `wayland-scanner` from XML
- Docs: `docs/wayland-area-selection.md` (full research doc),
  `docs/wayland-book/` submodule (The Wayland Book)

## Roadmap (in order)

1. ~~Display enumeration~~ — DONE (see Status below)
2. Borderless/barless overlay window positioned to cover all displays correctly
   (wlr-layer-shell, OVERLAY layer, one surface per output)
3. Graphics context with hardware acceleration for drawing
4. Screenshot current display state (wlr-screencopy) and render it inside the window
5. Rectangular mouse selection, flameshot-style (bright inside region, dimmed outside)
6. Annotation tools: lines, arrows, rectangles (+ controls)
7. On Enter: encode final image + drawings, store on clipboard

## Current Status

### Step 1: Display Enumeration — DONE

`src/app/wayland_main.c` connects to compositor, binds all `wl_output` globals (v4 for
the `name` event), prints name/position/resolution/scale, cleans up.

Machine's actual outputs (all report position 0,0 — compositor quirk to handle later):

| Name       | Resolution | Scale | Notes            |
|------------|-----------|-------|------------------|
| `eDP-1`    | 1920x1200 | 2     | laptop, 16:10    |
| `DP-3`     | 1920x1080 | 1     | FHD 16:9         |
| `HDMI-A-1` | 3440x1440 | 1     | ultrawide WQHD   |

Key learnings:
- Two `wl_display_roundtrip`s needed: first discovers/binds globals, second receives
  output events. One-shot pattern, no event loop yet.
- Mixed scale factors (2 vs 1) across outputs = mixed DPI problem deferred until after
  basic pipeline works.

### Next Step: Step 2 — Layer-Shell Overlay Window

Open borderless fullscreen windows covering each output via
`zwlr_layer_shell_unstable_v1`:
- Bind layer_shell + compositor + seat globals from registry
- Per output: `wl_surface` → `get_layer_surface` (layer `OVERLAY`, anchor all edges,
  exclusive zone `-1`)
- Keyboard interactivity `exclusive` (needed later for Escape/Enter)
- Need an event loop now (`wl_display_dispatch` / `wl_display_get_fd` + poll)
- First render can be a solid color buffer via `wl_shm` to prove positioning works;
  wl_buffer attach + commit, wait for frame callbacks

Questions to resolve during Step 2:
- Hardware acceleration choice for step 3: OpenGL ES via EGL on wl_surface vs software
  Cairo on shm buffers (cairo-gl possible). Decide after basic overlay works.

## Gotchas & Decisions Log

- Compositor reports all output positions as 0,0 → unified bounding-box math must not
  assume real positions; may need `xdg-output-manager` (zwlr layer doesn't fix logical
  layout) or accept overlap behavior as-is initially.
- `wlr-screencopy-unstable-v1` chosen over portal/ext-image-copy-capture (broadest
  support on wlroots compositors; see research doc for tradeoffs).
- Multi-monitor plan: one layer surface per output; selection coords in logical space;
  unified bounding-box capture buffer with black-filled gaps.
