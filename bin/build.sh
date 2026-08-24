#!/bin/bash

mkdir -p data
pushd ./data

wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ../src/os/generated/xdg-shell-client-protocol.h
wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ../src/os/generated/xdg-shell-client-protocol.c

gcc $(realpath ../src/app/wayland_main.c) -o inkshot \
    -DBUILD_SLOW=1 \
    $(pkg-config --cflags --libs wayland-client)


# odin build ../src -out:inkshot -debug -collection:lib=../lib -error-pos-style:unix

popd
