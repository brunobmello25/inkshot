package inkshot

import "core:fmt"
import "core:log"
import wl "lib:odin-wayland"

main :: proc() {
	fmt.println("Hello, Inkshot!")
	context.logger = log.create_console_logger()

	display := wl.display_connect(nil)
	if display == nil {
		log.panicf("Failed to connect to Wayland display")
	}

	registry := wl.display_get_registry(display)
	if registry == nil {
		log.panicf("Failed to get Wayland registry")
	}

	log.infof("Connected to Wayland display: %v", display)
}
