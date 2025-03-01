#include "keyboard.h"

const int transpose = -12;

int keymap_chromatic_piano(int key, int shift) {
    const int map[16] = {61,63, 0,66,68,70, 0,73,
                         60,62,64,65,67,69,71,72};
    if (map[key] == 0) return 0;
    return map[key] + transpose + 12*shift;
}

int keymap_pentatonic_linear(int key, int shift) {
    const int map[16] = {61,63,66,68,70,73,75,78,
                         80,82,85,87,90,92,94,97};
    if (map[key] == 0) return 0;
    return map[key] + transpose + 36*shift;    
}

int keymap_pentatonic_zigzag(int key, int shift) {
    const int map[16] = {63,68,73,78,82,87,92,97,
                         61,66,70,75,80,85,90,94};
    if (map[key] == 0) return 0;
    return map[key] + transpose + 36*shift;    
}

