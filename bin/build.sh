#!/bin/bash

mkdir -p target

pushd ./target

gcc ../src/main.c -o inkshot \
    -DBUILD_SLOW=1 \
    $(pkg-config --cflags --libs wayland-client)

popd
