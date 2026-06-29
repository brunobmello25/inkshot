package inkshot

import "core:c"
import "core:log"
import sdl "vendor:sdl3"

main :: proc() {
	context.logger = log.create_console_logger()

	if !sdl.Init(sdl.INIT_VIDEO) {
		log.panicf("Failed to initialize SDL: %s", sdl.GetError())
	}

	bounds_x, bounds_y, ok := get_overlay_bounds()

	log.debugf("Overlay bounds: x=%d, y=%d, ok=%v", bounds_x, bounds_y, ok)
}

get_overlay_bounds :: proc() -> ([2]int, [2]int, bool) {
	display_count: c.int
	diplay_ids := sdl.GetDisplays(&display_count)

	min_x, min_y := max(int), max(int)
	max_x, max_y := min(int), min(int)

	for i in 0 ..< display_count {
		bounds: sdl.Rect
		ok := sdl.GetDisplayBounds(diplay_ids[i], &bounds)
		if !ok {
			return 0, 0, false
		}

		if int(bounds.x) < min_x {
			min_x = int(bounds.x)
		}
		if int(bounds.y) < min_y {
			min_y = int(bounds.y)
		}
		if int(bounds.x + bounds.w) > max_x {
			max_x = int(bounds.x + bounds.w)
		}
		if int(bounds.y + bounds.h) > max_y {
			max_y = int(bounds.y + bounds.h)
		}
	}

	return {min_x, min_y}, {max_x, max_y}, true
}
