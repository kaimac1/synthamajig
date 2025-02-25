#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "ngl.h"

void draw_gauge_xy(int x, int y, int value, int minval, int maxval, const char* text);

void draw_gauge_param(int pos, int value, const char *text);
void draw_gauge_custom(int pos, int value, int minval, int maxval, const char *text);

#ifdef __cplusplus
}
#endif
