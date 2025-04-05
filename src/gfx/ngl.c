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

static inline __attribute__((always_inline)) void fast_bitmap_loop(unsigned int x, unsigned int y, unsigned int width, unsigned int height, uint8_t *data, 
unsigned int colbytes, int lastrowy, int shift) {

    for (unsigned int by=0; by<colbytes; by++) {
        const bool final_partial_row = (by == colbytes-1) && lastrowy;
        const size_t rowstart = by * width;
        uint8_t *rowdata = &data[rowstart];
        size_t fbrow = NGL_DISPLAY_WIDTH * (y/8) + x;

        if (shift == 0) {
            // Page-aligned
            if (final_partial_row) {
                const uint8_t mask = 0xFF << lastrowy;
                for (int px=0; px<width; px++) {
                    framebuffer[fbrow + px] &= mask; // clear top num pixels
                    framebuffer[fbrow + px] |= rowdata[px] & ~mask;
                }
            } else {
                for (int px=0; px<width; px++) {
                    framebuffer[fbrow + px] = rowdata[px];
                }
            }
        } else {
            // Not aligned, need to shift
            const uint8_t mask = 0xFF << shift;
            if (!final_partial_row) {
                for (int px=0; px<width; px++) {
                    uint16_t buf = rowdata[px] << shift;
                    framebuffer[fbrow + px] &= ~mask;
                    framebuffer[fbrow + px] |= buf & 0xFF;
                    framebuffer[fbrow + NGL_DISPLAY_WIDTH + px] &= mask;
                    framebuffer[fbrow + NGL_DISPLAY_WIDTH + px] |= buf >> 8;                    
                }
            } else {
                if (lastrowy + shift <= 8) {
                    // Final partial row fits on a single page
                    const uint8_t endmask = ~(0xFF >> (8 - shift - lastrowy)) | ~mask;
                    for (int px=0; px<width; px++) {
                        uint8_t buf = rowdata[px] << shift;
                        framebuffer[fbrow + px] &= endmask;
                        framebuffer[fbrow + px] |= buf & ~endmask;
                    }
                } else {
                    // Final partial row covers two pages
                    const uint8_t endmask = 0xFF << (shift + lastrowy - 8);
                    for (int px=0; px<width; px++) {
                        uint16_t buf = rowdata[px] << shift;
                        framebuffer[fbrow + px] &= ~mask;
                        framebuffer[fbrow + px] |= buf & 0xFF;
                        framebuffer[fbrow + NGL_DISPLAY_WIDTH + px] &= endmask;
                        framebuffer[fbrow + NGL_DISPLAY_WIDTH + px] |= (buf >> 8) & ~endmask;
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
    const int last_row_y_pixels = font->height & 0x07; 
    const int shift = y & 0b111; 
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

        fast_bitmap_loop(xoffs, y, char_width, font->height, (uint8_t*)&font->data[idx],
            bytes_per_col, last_row_y_pixels, shift);

        xoffs += char_width + font->char_spacing;
    }
}

// void ngl_text_old(nglFont *font, int x, int y, uint8_t flags, const char* text) {

//     const int len = strlen(text);
//     const size_t bytes_per_col = (font->height+7)/8;
//     const bool invert = flags & TEXT_INVERT;

//     int xoffs = x;

//     if (flags & TEXT_CENTRE || flags & TEXT_ALIGN_RIGHT) {
//         // Need to get total text length
//         int xlen = 0;
//         for (int i=0; i<len; i++) {
//             const uint8_t c = text[i] - font->first_char;
//             const uint8_t char_width = font->widths[c];
//             xlen += char_width + font->char_spacing;
//         }
//         // Adjust start position
//         if (flags & TEXT_CENTRE) {
//             xoffs -= xlen/2;
//         } else {
//             xoffs -= xlen;
//         }
//     }

//     for (int i=0; i<len; i++) {
//         const uint8_t c = text[i] - font->first_char;
//         const uint8_t char_width = font->widths[c];
//         size_t idx = font->index[c];

//         if (invert) {
//             for (int px=0; px<char_width; px++) {
//                 uint8_t data[bytes_per_col];
//                 for (int b=0; b<bytes_per_col; b++) {
//                     data[b] = ~font->data[idx + b];
//                 }
//                 draw_column(xoffs+px, y, &data[0], font->height);
//                 idx += bytes_per_col;
//             }
//         } else {
//             for (int px=0; px<char_width; px++) {
//                 // draw directly from font data
//                 draw_column(xoffs+px, y, (uint8_t*)&font->data[idx], font->height);
//                 idx += bytes_per_col;
//             }
//         }

//         xoffs += char_width + font->char_spacing;
//     }
// }
