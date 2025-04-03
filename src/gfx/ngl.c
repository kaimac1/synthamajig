// nicegl
// Copyright (C) 2024 kaimac.org

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ngl.h"
#include "font.h"

uint8_t framebuffer[NGL_FRAMEBUFFER_SIZE];

nglFont font_minipixel = {32, 96, 12, 1, font_minipixel_index, font_minipixel_data};


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

// Draw a vertical column of up to 128 pixels starting at x,y
void draw_column(int x, int y, uint8_t *data, int num) {
    int idx = NGL_DISPLAY_WIDTH * (y/8) + x;
    int py = y%8; // starting y-pixel within page
    int bits = 8-py;
    int b=0;

    if (bits <= 8) {
        // Initial page (full or partial)
        framebuffer[idx] &= 0xFF >> bits;
        framebuffer[idx] |= data[b] << py;
        num -= bits;
        idx += NGL_DISPLAY_WIDTH;
    }

    while (num >= 8) {
        // Full page
        framebuffer[idx] = (data[b] >> bits) | (data[b+1] << py);
        b++;
        num -= 8;
        idx += NGL_DISPLAY_WIDTH;
    }

    if (num > 0) {
        // Final partial page
        uint8_t mask = 0xFF << num;
        framebuffer[idx] &= mask;
        framebuffer[idx] |= ((data[b] >> bits) | (data[b+1] << py)) & ~mask;
    }
}

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
    int dx = abs(x1 - x0), dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
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
    // for (int px=0; px<w; px++) {
    //     for (int py=0; py<h; py++) {
    //         draw_pixel(x+px, y+py, data[px*h/8 + py/8] & (1<<(py%8)));
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

    int xoffs = x;
    int len = strlen(text);

    if (flags & TEXT_CENTRE || flags & TEXT_ALIGN_RIGHT) {
        int xlen = 0;
        for (int i=0; i<len; i++) {
            uint8_t c = text[i] - font->first_char;
            const uint8_t char_width = (font->index[c+1] - font->index[c]) / 2;
            xlen += char_width + font->char_spacing;
        }
        if (flags & TEXT_CENTRE) {
            xoffs -= xlen/2;
        } else {
            xoffs -= xlen;
        }
    }

    for (int i=0; i<len; i++) {
        uint8_t c = text[i] - font->first_char;
        const uint8_t char_width = (font->index[c+1] - font->index[c]) / 2;

        for (int px=0; px<char_width; px++) {
            uint8_t data[2];
            data[0] = font->data[font->index[c] + px]; // top 8 pixels
            data[1] = font->data[font->index[c] + char_width + px] >> 4; // bottom 4 pixels
            if (flags & TEXT_INVERT) {
                data[0] = ~data[0];
                data[1] = ~data[1];
            }
            draw_column(xoffs+px, y, &data[0], font->height);
        }

        xoffs += char_width + font->char_spacing;
    }
}
