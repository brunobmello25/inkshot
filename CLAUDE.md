# inkshot

Wayland-native Flameshot replacement, written in C.

## Architecture: Freeze-and-Select

1. **Capture** all outputs via `wlr-screencopy` protocol into buffers
2. **Overlay** the frozen captures on `wlr-layer-shell` surfaces (one per output, `OVERLAY` layer, exclusive keyboard grab)
3. **Select** — user drags a rectangle; dimmed image with bright selection region
4. **Crop & output** — cut the selection, encode to PNG, copy to clipboard or save

## Multi-Monitor

3 monitors: FHD 16:10, FHD 16:9, WQHD 3440x1440 (different ratios/resolutions).
Unified bounding-box buffer (minx,miny to maxx,maxy), black fill for gaps.
One layer-shell surface per output, selection coordinates in logical space.

## Tech Stack

- Language: C (not Rust)
- Build: `bin/build.sh` (GCC), output to `target/inkshot`
- Core libs: `libwayland-client`, `cairo`, `xkbcommon`
- Protocol bindings generated with `wayland-scanner` from XML files
- No higher-level toolkit — direct libwayland-client like slurp/grim
- Reference implementations: `slurp` (selection), `grim` (capture)

## Project Layout

- `src/main.c` — entry point
- `src/base.h` — type aliases (u8/u16/.../i32/f32 etc), macros (ASSERT, ARRAY_COUNT, KB/MB/GB), build flags
- `bin/build.sh` — build script
- `docs/` — research docs + wayland-book submodule
- `target/` — build output (gitignored)

## Progress

### Step 1: Display Enumeration — DONE

`src/main.c` connects to the Wayland compositor, binds all `wl_output` globals, and prints each output's name, logical position, resolution, and scale factor.

Actual outputs on this machine:
- `eDP-1`    — 1920x1200, pos 0,0, scale 2  (laptop display, FHD 16:10)
- `DP-3`     — 1920x1080, pos 0,0, scale 1  (FHD 16:9)
- `HDMI-A-1` — 3440x1440, pos 0,0, scale 1  (ultrawide WQHD)

All positions are 0,0 — compositor is reporting that. Will matter when we compute the unified bounding box.

Key implementation notes:
- Bind `wl_output` at version 4 (needed for the `name` event)
- Two `wl_display_roundtrip` calls required: first discovers/binds globals, second receives the output events (geometry, mode, scale, name, done). No event loop needed — this is a one-shot sync pattern.
- State structs use PascalCase (`Output_Info`, `State`) per the code style in this repo.

### Next Step: Step 2 — Screencopy (wlr-screencopy)

Capture each output's framebuffer into a shared-memory buffer using the `zwlr_screencopy_manager_v1` protocol.
- Protocol XML: need to obtain `wlr-screencopy-unstable-v1.xml` and run `wayland-scanner` to generate bindings
- Allocate a `wl_shm` buffer per output (width × height × 4 bytes, ARGB8888 or XRGB8888)
- Call `zwlr_screencopy_manager_v1_capture_output`, wait for the `ready` event
- After ready, the pixel data is in the shm buffer — keep it alive for the overlay step
