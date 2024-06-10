#!/bin/sh

set -xe

gcc -o png_parser src/main.c src/png.c src/image.c -Wall -Wextra -lraylib -lm -lz -ggdb
