#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

/*** Config ***/
#define NGL_DISPLAY_WIDTH  128
#define NGL_DISPLAY_HEIGHT 128

//#define FRAMEBUFFER_BOUNDS_CHECK
/******/

#define NGL_FRAMEBUFFER_SIZE ((NGL_DISPLAY_WIDTH) * (NGL_DISPLAY_HEIGHT) / 8)

// Option flags for ngl_text
#define TEXT_INVERT         0x01
#define TEXT_CENTRE         0x02
#define TEXT_ALIGN_RIGHT    0x04

typedef struct {
    uint8_t first_char;
    uint8_t num_chars;
    uint8_t height;
    uint8_t char_spacing;
    const uint16_t *index;
    const uint8_t *widths;
    const uint8_t *data;
} nglFont;

typedef struct {
    int32_t width;
    int32_t height;
    const uint8_t *data;
} nglBitmap;

typedef enum {
    FILLCOLOUR_BLACK = 0,
    FILLCOLOUR_WHITE,
    FILLCOLOUR_HALF
} nglFillColour;


void ngl_init(void);

// Get the framebuffer
uint8_t *ngl_framebuffer(void);

// Draw single pixel
void ngl_setpixel(int x, int y, bool colour);

// Fill whole screen with colour
void ngl_fillscreen(bool colour);

// Draw filled rectangle
void ngl_rect(int x, int y, int w, int h, nglFillColour fillcolour);

// Draw line (Bresenham)
void ngl_line(int x0, int y0, int x1, int y1, bool colour);

void ngl_bitmap(int x, int y, nglBitmap bitmap);

// draw bitmap that can be partially off either side of the screen
// (x may be negative, and x+width may be beyond the display width)
void ngl_bitmap_xclip(int x, int y, nglBitmap bitmap);

// draw text
void ngl_text(nglFont *font, int x, int y, uint8_t flags, const char* text);
void ngl_textf(nglFont *font, int x, int y, uint8_t flags, const char *fmt, ...);


#ifdef __cplusplus
}
#endif
