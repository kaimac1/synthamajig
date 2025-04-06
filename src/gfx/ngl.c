// nicegl
// Copyright (C) 2024 kaimac.org

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ngl.h"
#include "font.h"

uint8_t framebuffer[NGL_FRAMEBUFFER_SIZE];

nglFont font_minipixel = {32, 96, 12, 1, font_minipixel_index, font_minipixel_widths, font_minipixel_data};
nglFont font_notalot = {32, 95, 9, 1, font_notalot_index, font_notalot_widths, font_notalot_data};


void ngl_init(void) {
    //
}

uint8_t *ngl_framebuffer(void) {
    return framebuffer;
}

void ngl_setpixel(unsigned int x, unsigned int y, bool colour) {
#ifdef FRAMEBUFFER_BOUNDS_CHECK
    if ((x >= NGL_DISPLAY_WIDTH) || (y >= NGL_DISPLAY_HEIGHT)) return;
#endif

    const int idx = NGL_DISPLAY_WIDTH * (y/8) + x;
    uint8_t mask = 1 << (y & 0x07);
    if (colour) {
        framebuffer[idx] |= mask;
    } else {
        framebuffer[idx] &= ~mask;
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

static inline __attribute__((always_inline)) void fb_and_or(size_t offset, uint8_t mask, uint8_t data) {
    framebuffer[offset] &= mask;
    framebuffer[offset] |= data;
}

static inline __attribute__((always_inline)) void fast_bitmap_loop(unsigned int x, unsigned int y, unsigned int width, unsigned int height, uint8_t *data, 
unsigned int colbytes, int lastrowy, int shift) {

    for (unsigned int by=0; by<colbytes; by++) {
        const bool final_partial_row = (by == colbytes-1) && lastrowy;
        uint8_t *rowdata = &data[by*width];
        size_t fbrow = NGL_DISPLAY_WIDTH * (y/8) + x;

        if (shift == 0) {
            // Page-aligned
            if (final_partial_row) {
                const uint8_t mask = 0xFF << lastrowy;
                for (int px=0; px<width; px++) {
                    fb_and_or(fbrow + px, mask, rowdata[px] & ~mask);
                }
            } else {
                for (int px=0; px<width; px++) {
                    uint8_t pdata = rowdata[px];
                    framebuffer[fbrow + px] = rowdata[px];
                }
            }
        } else {
            // Not aligned, need to shift
            const uint8_t mask = 0xFF << shift;
            if (!final_partial_row) {
                for (int px=0; px<width; px++) {
                    uint32_t buf = rowdata[px] << shift;
                    fb_and_or(fbrow + px, ~mask, buf);
                    fb_and_or(fbrow + NGL_DISPLAY_WIDTH + px, mask, buf >> 8);
                }
            } else {
                if (lastrowy + shift <= 8) {
                    // Final partial row fits on a single page
                    const uint8_t endmask = ~(0xFF >> (8 - shift - lastrowy)) | ~mask;
                    for (int px=0; px<width; px++) {
                        uint32_t buf = rowdata[px] << shift;
                        fb_and_or(fbrow + px, endmask, buf & ~endmask);
                    }
                } else {
                    // Final partial row covers two pages
                    const uint8_t endmask = 0xFF << (shift + lastrowy - 8);
                    for (int px=0; px<width; px++) {
                        uint32_t buf = rowdata[px] << shift;
                        fb_and_or(fbrow + px, ~mask, buf);
                        fb_and_or(fbrow + NGL_DISPLAY_WIDTH + px, endmask, (buf >> 8) & ~endmask);
                    }
                }
            }
        }
        y += 8;
    }
}


// Bitmaps

void ngl_bitmap(int x, int y, nglBitmap bitmap) {
    const unsigned int colbytes = ((bitmap.height+7)/8);
    const int lastrowy = bitmap.height & 0x07;
    const int shift = y & 0x07;
    fast_bitmap_loop(x, y, bitmap.width, bitmap.height, (uint8_t*)bitmap.data, colbytes, lastrowy, shift);
}

// void ngl_bitmap_xclip(int x, int y, nglBitmap bitmap) {
//     int w = bitmap.width;
//     int xoffset = 0;
//     if (x < 0) {
//         xoffset = -x;
//         w -= xoffset;
//         x = 0;
//     }
//     if (x+w > NGL_DISPLAY_WIDTH) {
//         w = NGL_DISPLAY_WIDTH - x;
//     }
//     for (int px=0; px<w; px++) {
//         int colstart = px * ((bitmap.height+7)/8);
//         draw_column(x+px, y, (uint8_t*)&bitmap.data[colstart], bitmap.height);
//     }    
// }



// Text

static inline __attribute__((always_inline)) void fast_font_loop(unsigned int x, unsigned int y, unsigned int width, unsigned int height, uint8_t *data, 
unsigned int colbytes, int lastrowy, int shift) {


}



void ngl_textf(nglFont *font, int x, int y, uint8_t flags, const char *fmt, ...) {
    const size_t TEXTF_BUFFER_SIZE = 128;
    char str[TEXTF_BUFFER_SIZE];
    va_list args;
    va_start (args, fmt);
    vsnprintf(str, sizeof(str), fmt, args);
    ngl_text(font, x, y, flags, str);
    va_end(args);
}

#define GET_C()      const uint8_t c = text[i] - font->first_char;
#define CHAR_WIDTH() const uint8_t char_width = font->widths[c];
#define DATA()       uint8_t *data = (uint8_t *)&font->data[font->index[c] + by*char_width];
#define GET_CHAR()   GET_C(); CHAR_WIDTH(); DATA();
#define NEXT_CHAR()  fbidx += char_width + font->char_spacing;


void ngl_text(nglFont *font, int x, int y, uint8_t flags, const char* text) {

    const int len = strlen(text);
    int xstart = x;

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
            xstart -= xlen/2;
        } else {
            xstart -= xlen;
        }
    }

    const size_t bytes_per_col = (font->height+7)/8;
    const int last_row_y_pixels = font->height & 0x07; 
    const int shift = y & 0b111;
    const uint8_t datainv = flags & TEXT_INVERT ? 0xFF : 0x00;
    const uint32_t mask = 0xFF << shift;

    for (unsigned int by=0; by<bytes_per_col; by++) {
        const bool final_partial_row = (by == bytes_per_col-1) && last_row_y_pixels;
        size_t fbidx = NGL_DISPLAY_WIDTH * (y/8) + xstart;

        if (shift == 0) {
            // Page-aligned
            uint32_t fbmask = 0xFF;
            uint32_t datamask = 0xFF;
            if (final_partial_row) {
                fbmask = 0xFF << last_row_y_pixels;
                datamask = ~fbmask;
            }
            for (int i=0; i<len; i++) {
                GET_CHAR();
                for (int px=0; px<char_width; px++) {
                    fb_and_or(fbidx + px, fbmask, (data[px] ^ datainv) & datamask);
                }
                NEXT_CHAR();
            }

        } else {
            // Not aligned, need to shift
            if (!final_partial_row) {
                for (int i=0; i<len; i++) {
                    GET_CHAR();
                    for (int px=0; px<char_width; px++) {
                        uint32_t buf = (data[px] ^ datainv) << shift;
                        fb_and_or(fbidx + px, ~mask, buf);
                        fb_and_or(fbidx + NGL_DISPLAY_WIDTH + px, mask, buf >> 8);
                    }
                    NEXT_CHAR();
                }
            } else {
                for (int i=0; i<len; i++) {
                    GET_CHAR();
                    if (last_row_y_pixels + shift <= 8) {
                        // Final partial row fits on a single page
                        const uint32_t endmask = ~(0xFF >> (8 - shift - last_row_y_pixels)) | ~mask;
                        for (int px=0; px<char_width; px++) {
                            uint32_t buf = (data[px] ^ datainv) << shift;
                            fb_and_or(fbidx + px, endmask, buf & ~endmask);
                        }
                    } else {
                        // Final partial row covers two pages
                        const uint32_t endmask = 0xFF << (shift + last_row_y_pixels - 8);
                        for (int px=0; px<char_width; px++) {
                            uint32_t buf = (data[px] ^ datainv) << shift;
                            fb_and_or(fbidx + px, ~mask, buf);
                            fb_and_or(fbidx + NGL_DISPLAY_WIDTH + px, endmask, (buf >> 8) & ~endmask);
                        }
                    }
                    NEXT_CHAR();
                }
            }
        }
        y += 8;
    }
}
