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
- `bin/build.sh` — build script
- `docs/` — research docs + wayland-book submodule
- `target/` — build output (gitignored)
