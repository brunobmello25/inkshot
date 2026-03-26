#!/bin/bash

mkdir -p target

pushd ./target

gcc ../src/main.c -o inkshot

popd
