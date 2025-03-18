#!/bin/bash
cd build
make synth
arm-none-eabi-size synth.elf
arm-none-eabi-nm --size-sort --reverse-sort -S -l -C -A synth.elf > nm.txt