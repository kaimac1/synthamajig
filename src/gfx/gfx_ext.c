// Extra graphics routines

#include "gfx_ext.h"
#include "assets/assets.h"
#include <math.h>

#define PI 3.141593f

void draw_gauge_xy(int x, int y, int value, int minval, int maxval, const char* text) {
    
    const int radius = 12;
    if (value < minval) value = minval;
    if (value > maxval) value = maxval;
    float theta=-4.3f + 5.5f*(float)value/maxval; //FIXME assumes minval is zero
    
    ngl_bitmap(x-16,y-16, gauge_arc);
    ngl_line(x,y, x+radius*cos(theta),y+radius*sin(theta), 1);
    draw_text(x,y+16,TEXT_CENTRE,text);
}

void draw_gauge_custom(int pos, int value, int minval, int maxval, const char *text) {
    const int spacing=24;
    int x=0, y=0;
    if (pos == 0) {x=-1; y=-1;}
    if (pos == 1) {x= 1; y=-1;}
    if (pos == 2) {x=-1; y= 1;}
    if (pos == 3) {x= 1; y= 1;}
    draw_gauge_xy(64+x*spacing, 64+y*spacing, value, minval, maxval, text);    
}

void draw_gauge_param(int pos, int value, const char *text) {
    const int spacing=24;
    int x=0, y=0;
    if (pos == 0) {x=-1; y=-1;}
    if (pos == 1) {x= 1; y=-1;}
    if (pos == 2) {x=-1; y= 1;}
    if (pos == 3) {x= 1; y= 1;}
    draw_gauge_xy(64+x*spacing, 64+y*spacing, value, 0, 127, text);
}