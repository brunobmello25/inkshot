#!/bin/bash

mkdir -p data
pushd ./data

wayland-scanner client-header ../protocols/xdg-shell.xml ../src/os/generated/xdg-shell-client-protocol.h
wayland-scanner private-code ../protocols/xdg-shell.xml ../src/os/generated/xdg-shell-client-protocol.c

wayland-scanner client-header ../protocols/wlr-screencopy-unstable-v1.xml ../src/os/generated/wlr-screen-copy.h
wayland-scanner client-header ../protocols/wlr-screencopy-unstable-v1.xml ../src/os/generated/wlr-screen-copy.c

wayland-scanner client-header ../protocols/wlr-layer-shell-unstable-v1.xml ../src/os/generated/wlr-layer-shell.h
wayland-scanner client-header ../protocols/wlr-layer-shell-unstable-v1.xml ../src/os/generated/wlr-layer-shell.c

gcc $(realpath ../src/app/wayland_main.c) -o inkshot \
    -DBUILD_SLOW=1 \
    $(pkg-config --cflags --libs wayland-client)


# odin build ../src -out:inkshot -debug -collection:lib=../lib -error-pos-style:unix

popd
