# Wayland Area Selection — Research & Implementation Guide

## The Problem

Wayland's security model forbids clients from directly accessing screen content or
grabbing global input. Unlike X11 (where Flameshot works natively), a Wayland screenshot
tool must use explicit compositor cooperation through well-defined protocols.

## Capture Mechanisms

### 1. `wlr-screencopy-unstable-v1` (Current standard for wlroots compositors)

Used by `grim`, `wayshot`, and most Wayland screenshot tools on Sway/Hyprland/River.

**Flow:**
1. Bind to `zwlr_screencopy_manager_v1` via the Wayland registry
2. Call `capture_output_region(cursor, wl_output, x, y, w, h)` for a region
3. Compositor sends `buffer` events describing supported formats (format, width, height, stride)
4. Client allocates a matching `wl_shm` buffer
5. Client calls `copy(buffer)` on the frame
6. Compositor sends `ready` (with timestamp) or `failed`

**Status:** Deprecated in favor of `ext-image-copy-capture-v1`, but remains the most
widely supported protocol in practice (early 2026).

### 2. `ext-image-copy-capture-v1` (New official standard)

The successor merged into wayland-protocols. Improvements over wlr-screencopy:

- Session-based capture model
- Better format negotiation (compositor advertises constraints dynamically)
- Dedicated cursor sessions with position/hotspot tracking
- Explicit damage regions for efficient partial updates
- Built-in presentation timestamps

**Adoption:** Supported by wlroots, grim (newer versions), Hyprland. Not yet universal.

### 3. `xdg-desktop-portal` Screenshot Portal

**D-Bus interface:** `org.freedesktop.portal.Screenshot`

Works on all compositors (GNOME, KDE, wlroots) but gives no control over the UI.
Region selection behavior depends on the portal backend. Returns a file URI, not raw
pixel buffers.

Portal backends by compositor:
- GNOME: `xdg-desktop-portal-gnome`
- wlroots (Sway): `xdg-desktop-portal-wlr` (uses `slurp` for region selection)
- Hyprland: `xdg-desktop-portal-hyprland`
- KDE: `xdg-desktop-portal-kde`

### 4. PipeWire Screencast

Continuous screen capture via `org.freedesktop.portal.ScreenCast`. Overkill for
single screenshots — used by OBS, Firefox for screen sharing.

## Area Selection: The Layer Shell Overlay

### Protocol: `wlr-layer-shell-unstable-v1`

This is how `slurp`, `waysip`, and `watershot` let the user draw a selection rectangle.

**Key concepts:**
- Creates surfaces in z-depth layers: `background`, `bottom`, `top`, `overlay`
- The `overlay` layer renders above everything including fullscreen windows
- Surfaces can be anchored to all edges and sized to cover the full output

**How it works (slurp's approach):**

1. Bind `zwlr_layer_shell_v1` from the Wayland registry
2. For each output, create a `wl_surface` and a layer surface:
   - Layer: `OVERLAY`
   - Anchor: all four edges (full coverage)
   - Exclusive zone: `-1` (extend to edges, no space reservation)
   - Keyboard interactivity: `exclusive` (grabs keyboard for Escape to cancel)
3. Render a semi-transparent overlay (e.g., with Cairo)
4. Handle `wl_pointer` events: track enter/motion/button to compute the selection rectangle
5. On button release, output the geometry (`x,y WxH`)

## Recommended Architecture: Freeze-and-Select

The most polished screenshot tools use this two-phase approach:

### Phase 1 — Capture
Take a full-screen screenshot of all outputs using screencopy. The screen content is
now frozen in a buffer.

### Phase 2 — Overlay & Select
Display the captured screenshot as a layer-shell overlay on the `overlay` layer. The
screen appears frozen to the user. Let them draw a rectangle on the frozen image, dimming
non-selected areas.

### Phase 3 — Crop & Output
Crop the captured buffer to the selected region. Encode to PNG and copy to clipboard or
save to file.

This is what `watershot`, `wayshot --freeze`, and `hyprshot --freeze` do. It prevents
screen content from changing during selection.

## Reference Implementations

| Tool | Language | Capture | Selection | Notes |
|------|----------|---------|-----------|-------|
| [slurp](https://github.com/emersion/slurp) | C | — | Layer shell | Reference for area selection |
| [grim](https://github.com/emersion/grim) | C | wlr-screencopy / ext-image-copy-capture | — | Reference for screen capture |
| [wayshot](https://github.com/waycrate/wayshot) | Rust | wlr-screencopy (`libwayshot`) | Layer shell (`libwaysip`) | All-Rust, no external deps |
| [waysip](https://github.com/waycrate/waysip) | Rust | — | Layer shell | Standalone area selector library |
| [watershot](https://github.com/Kirottu/watershot) | Rust | grim (external) | Layer shell + WGSL shaders | Polished UI |
| [shotman](https://sr.ht/~whynothugo/shotman/) | Rust | wlr-screencopy | Layer shell | Minimal |

## Protocols Needed

| Protocol | Purpose | Required? |
|----------|---------|-----------|
| `wlr-layer-shell-unstable-v1` | Selection overlay | Yes |
| `wlr-screencopy-unstable-v1` | Screen capture (broad compat) | Yes |
| `ext-image-copy-capture-v1` | Screen capture (future-proof) | Optional |
| `wl_shm` | Buffer allocation | Yes |
| `wl_output` + `xdg-output-manager` | Output discovery + logical coords | Yes |
| `org.freedesktop.portal.Screenshot` | Portal fallback (GNOME/KDE) | Optional |

## C Libraries

For a C implementation, the relevant libraries are:

- **`libwayland-client`** — Core Wayland client library
- **`wlr-protocols`** — Protocol XML files for wlr extensions (screencopy, layer-shell)
- **`cairo`** — 2D rendering for the overlay and selection rectangle
- **`wayland-scanner`** — Generates C bindings from protocol XML files
- **`xkbcommon`** — Keyboard handling (for Escape to cancel, etc.)
- **`libpng`** or **`stb_image_write`** — Image encoding

### Building Protocol Bindings

Wayland protocols are defined as XML files. Use `wayland-scanner` to generate C headers
and glue code:

```sh
# Generate client header
wayland-scanner client-header \
  /usr/share/wayland-protocols/staging/ext-image-copy-capture/ext-image-copy-capture-v1.xml \
  ext-image-copy-capture-v1-client-protocol.h

# Generate implementation
wayland-scanner private-code \
  /usr/share/wayland-protocols/staging/ext-image-copy-capture/ext-image-copy-capture-v1.xml \
  ext-image-copy-capture-v1-client-protocol.c

# wlr protocols (install wlr-protocols package or clone the repo)
wayland-scanner client-header \
  protocols/wlr-layer-shell-unstable-v1.xml \
  wlr-layer-shell-unstable-v1-client-protocol.h

wayland-scanner client-header \
  protocols/wlr-screencopy-unstable-v1.xml \
  wlr-screencopy-unstable-v1-client-protocol.h
```

### Minimal Flow in C (Pseudocode)

```c
// 1. Connect to Wayland display
struct wl_display *display = wl_display_connect(NULL);
struct wl_registry *registry = wl_display_get_registry(display);
// Listen for globals: wl_shm, wl_output, zwlr_screencopy_manager_v1,
//                     zwlr_layer_shell_v1

// 2. Capture full screen via screencopy
struct zwlr_screencopy_frame_v1 *frame =
    zwlr_screencopy_manager_v1_capture_output(screencopy_mgr, 0, output);
// Handle buffer/ready callbacks, copy pixels into shm buffer

// 3. Create layer-shell overlay
struct wl_surface *overlay_surface = wl_compositor_create_surface(compositor);
struct zwlr_layer_surface_v1 *layer_surface =
    zwlr_layer_shell_v1_get_layer_surface(
        layer_shell, overlay_surface, output,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "inkshot");
zwlr_layer_surface_v1_set_anchor(layer_surface, /* all edges */);
zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, -1);
zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface,
    ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);

// 4. Render frozen screenshot + selection UI with Cairo
// 5. Handle wl_pointer events for click-drag selection
// 6. On release, crop and save/copy
```

## Sources

- [Wayland Explorer — wlr-layer-shell](https://wayland.app/protocols/wlr-layer-shell-unstable-v1)
- [Wayland Explorer — wlr-screencopy](https://wayland.app/protocols/wlr-screencopy-unstable-v1)
- [Wayland Explorer — ext-image-copy-capture](https://wayland.app/protocols/ext-image-copy-capture-v1)
- [XDG Desktop Portal — Screenshot](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Screenshot.html)
- [Drew DeVault — Wayland shells](https://drewdevault.com/2018/07/29/Wayland-shells.html)
- [Arch Wiki — Screen capture](https://wiki.archlinux.org/title/Screen_capture)
