// nicegl
// Copyright (C) 2024 kaimac.org

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ngl.h"
#include "font.h"

//#define USE_SLOW_DRAW_COLUMN

uint8_t framebuffer[NGL_FRAMEBUFFER_SIZE];

nglFont font_minipixel = {32, 96, 12, 1, font_minipixel_index, font_minipixel_widths, font_minipixel_data};
nglFont font_notalot = {32, 95, 9, 1, font_notalot_index, font_notalot_widths, font_notalot_data};


void ngl_init(void) {
    //
}

uint8_t *ngl_framebuffer(void) {
    return framebuffer;
}

void ngl_setpixel(int x, int y, bool colour) {
#ifdef FRAMEBUFFER_BOUNDS_CHECK
    if ((x >= NGL_DISPLAY_WIDTH) || (y >= NGL_DISPLAY_HEIGHT)) return;
#endif

    int idx = NGL_DISPLAY_WIDTH * (y/8) + x;
    uint8_t bit = 1 << (y%8);
    if (colour) {
        framebuffer[idx] |= bit;
    } else {
        framebuffer[idx] &= ~bit;
    }
}

#ifdef USE_SLOW_DRAW_COLUMN
// slow!
static inline void draw_column(unsigned int x, unsigned int y, uint8_t *data, int num) {
    for (unsigned int dy=0; dy<num; dy++) {
        bool bit = data[dy/8] & (1 << (dy & 0x07));
        ngl_setpixel(x, y+dy, bit);
    }
}

#else
// fast!
// Draw a vertical column of up to 128 pixels starting at x,y
static inline void draw_column(unsigned int x, unsigned int y, uint8_t *data, int num) {
    const unsigned int py = y & 0b111; // starting y-pixel within page
    const unsigned int bits = 8 - py;
    size_t idx = NGL_DISPLAY_WIDTH * (y >> 3) + x;
    size_t b = 0;

    // TODO: handle num < 8

    if (py == 0) {
        // Page-aligned
        while (num >= 8) {
            framebuffer[idx] = data[b++];
            num -= 8;
            idx += NGL_DISPLAY_WIDTH;
        }
        if (num > 0) {
            const uint8_t mask = 0xFF << num;
            framebuffer[idx] &= mask;
            framebuffer[idx] |= data[b] & ~mask;
        }
        return;
    }

    // Not page-aligned so we have to shift everything

    union {
        struct {
            uint8_t lo;
            uint8_t hi;
        };
        uint16_t b16;
    } bytes;    

    // First page
    bytes.b16 = data[b++] << py;
    const uint8_t mask = 0xFF >> bits;
    framebuffer[idx] = (framebuffer[idx] & mask) | bytes.lo;
    num -= bits;
    idx += NGL_DISPLAY_WIDTH;

    // Full pages
    while (num >= 8) {
        bytes.b16 >>= py;
        bytes.hi = data[b++];
        bytes.b16 >>= bits;
        framebuffer[idx] = bytes.lo;
        num -= 8;
        idx += NGL_DISPLAY_WIDTH;
    }

    // Final partial page, if required
    if (num > 0) {
        bytes.b16 >>= py;
        if (num > py) bytes.hi = data[b++];
        bytes.b16 >>= bits;
        const uint8_t mask = 0xFF << num;
        framebuffer[idx] &= mask;   // clear top [num] pixels ([num] LSBs)
        framebuffer[idx] |= bytes.lo & ~mask;
    }
}
#endif

void ngl_fillscreen(bool colour) {
    memset(framebuffer, colour ? 0xFF : 0x00, sizeof(framebuffer));
}

void ngl_rect(int x, int y, int w, int h, nglFillColour fillcolour) {

    if ((x >= NGL_DISPLAY_WIDTH) || (y >= NGL_DISPLAY_HEIGHT)) return;
    if (y+h > NGL_DISPLAY_HEIGHT) {
        h = NGL_DISPLAY_HEIGHT - y - 1;
    }
    if (x+w > NGL_DISPLAY_WIDTH) {
        w = NGL_DISPLAY_WIDTH - x - 1;
    }

    if (fillcolour == FILLCOLOUR_HALF) {
        for (int xp=x; xp<x+w; xp++) {
            for (int yp=y; yp<y+h; yp++) {
                ngl_setpixel(xp, yp, ((xp ^ yp) & 1));
            }
        }
    } else {
        for (int xp=x; xp<x+w; xp++) {
            for (int yp=y; yp<y+h; yp++) {
                ngl_setpixel(xp, yp, fillcolour);
            }
        }
    }
}

void ngl_line(int x0, int y0, int x1, int y1, bool colour) {
    const int dx = abs(x1 - x0), dy = -abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    while (1) {
        ngl_setpixel(x0, y0, colour);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2*error;
        if (e2 >= dy) {
            error += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}



// Bitmaps

void ngl_bitmap(int x, int y, nglBitmap bitmap) {
    // //slow way for reference
    // for (int px=0; px<bitmap.width; px++) {
    //     for (int py=0; py<bitmap.height; py++) {
    //         ngl_setpixel(x+px, y+py, bitmap.data[px*bitmap.height/8 + py/8] & (1<<(py%8)));
    //     }
    // }

    for (int px=0; px<bitmap.width; px++) {
        // Columns are always a whole number of bytes so round up the height to next multiple of 8
        int colstart = px * ((bitmap.height+7)/8);
        draw_column(x+px, y, (uint8_t*)&bitmap.data[colstart], bitmap.height);
    }
}

void ngl_bitmap_xclip(int x, int y, nglBitmap bitmap) {
    int w = bitmap.width;
    int xoffset = 0;
    if (x < 0) {
        xoffset = -x;
        w -= xoffset;
        x = 0;
    }
    if (x+w > NGL_DISPLAY_WIDTH) {
        w = NGL_DISPLAY_WIDTH - x;
    }
    for (int px=0; px<w; px++) {
        int colstart = px * ((bitmap.height+7)/8);
        draw_column(x+px, y, (uint8_t*)&bitmap.data[colstart], bitmap.height);
    }    
}



// Text

void ngl_textf(nglFont *font, int x, int y, uint8_t flags, const char *fmt, ...) {
    const size_t TEXTF_BUFFER_SIZE = 128;
    char str[TEXTF_BUFFER_SIZE];
    va_list args;
    va_start (args, fmt);
    vsnprintf(str, sizeof(str), fmt, args);
    ngl_text(font, x, y, flags, str);
    va_end(args);
}

void ngl_text(nglFont *font, int x, int y, uint8_t flags, const char* text) {

    const int len = strlen(text);
    const size_t bytes_per_col = (font->height+7)/8;
    const bool invert = flags & TEXT_INVERT;

    int xoffs = x;

    if (flags & TEXT_CENTRE || flags & TEXT_ALIGN_RIGHT) {
        // Need to get total text length
        int xlen = 0;
        for (int i=0; i<len; i++) {
            const uint8_t c = text[i] - font->first_char;
            const uint8_t char_width = font->widths[c];
            xlen += char_width + font->char_spacing;
        }
        // Adjust start position
        if (flags & TEXT_CENTRE) {
            xoffs -= xlen/2;
        } else {
            xoffs -= xlen;
        }
    }

    for (int i=0; i<len; i++) {
        const uint8_t c = text[i] - font->first_char;
        const uint8_t char_width = font->widths[c];
        size_t idx = font->index[c];

        if (invert) {
            for (int px=0; px<char_width; px++) {
                uint8_t data[bytes_per_col];
                for (int b=0; b<bytes_per_col; b++) {
                    data[b] = ~font->data[idx + b];
                }
                draw_column(xoffs+px, y, &data[0], font->height);
                idx += bytes_per_col;
            }
        } else {
            for (int px=0; px<char_width; px++) {
                // draw directly from font data
                draw_column(xoffs+px, y, (uint8_t*)&font->data[idx], font->height);
                idx += bytes_per_col;
            }
        }

        xoffs += char_width + font->char_spacing;
    }
}
