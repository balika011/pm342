#!/usr/bin/env bash

# apt: sudo apt install libftdi-dev
# dnf: dnf dnf install libftdi1-devel && sed -i '6 s/-lftdi/-lftdi1/g' build.sh

g++ main.c pm342.c i2c.c mpsse.c -o pm342 -lftdi
