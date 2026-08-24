#!/bin/bash

mkdir -p data

pushd ./data

gcc $(realpath ../src/app/wayland_main.c) -o inkshot \
    -DBUILD_SLOW=1 \
    $(pkg-config --cflags --libs wayland-client)

# odin build ../src -out:inkshot -debug -collection:lib=../lib -error-pos-style:unix

popd
