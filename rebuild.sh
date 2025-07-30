#!/bin/bash
cd build
cmake -DPICO_BOARD=pimoroni_pico_plus2_rp2350 -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
