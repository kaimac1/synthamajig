#include <stddef.h>
#include <stdio.h>
#include "ngl.h"
#include "raylib.h"

#define DCOL ORANGE

Camera2D camera;
RenderTexture2D display;


void ngl_init(void) {
    camera.zoom = 1.0f;
    display = LoadRenderTexture(NGL_DISPLAY_WIDTH, NGL_DISPLAY_HEIGHT);
}

void ngl_sim_begin(void) {
    BeginTextureMode(display);
    BeginMode2D(camera);
}
void ngl_sim_end(void) {
    EndMode2D();
    EndTextureMode();
}




uint8_t *ngl_framebuffer(void) {
    return NULL;
}

void ngl_setpixel(int x, int y, bool colour) {
    DrawPixel(x,y, colour ? DCOL : BLACK);
}

void ngl_fillscreen(bool colour) {
    DrawRectangle(0,0, NGL_DISPLAY_WIDTH,NGL_DISPLAY_HEIGHT, colour ? DCOL : BLACK);
}

void ngl_rect(int x, int y, int w, int h, bool fillcolour) {
    DrawRectangle(x,y, w,h, fillcolour ? DCOL : BLACK);
}

void ngl_line(int x0, int y0, int x1, int y1, bool colour) {
    DrawLine(x0,y0, x1,y1, colour ? DCOL : BLACK);
}

static void draw_column(int x, int y, uint8_t *data, int num) {
    for (int n=0; n<num; n++) {
        DrawPixel(x,y+n, DCOL);
    }
}

void ngl_bitmap(int x, int y, nglBitmap bitmap) {
    DrawTexture(*(Texture2D *)bitmap.data, x,y, DCOL);

    // //slow way
    // for (int px=0; px<bitmap.width; px++) {
    //     for (int py=0; py<bitmap.height; py++) {
    //         int colstart = px * ((bitmap.height+7)/8);
    //         DrawPixel(x+px, y+py, (bitmap.data[colstart + py/8] & (1<<(py%8))) ? DCOL : BLACK);
    //     }
    // }

    // for (int px=0; px<bitmap.width; px++) {
    //     // Columns are always a whole number of bytes so round up the height to next multiple of 8
    //     int colstart = px * ((bitmap.height+7)/8);
    //     draw_column(x+px, y, (uint8_t*)&bitmap.data[colstart], bitmap.height);
    // }
}

void ngl_bitmap_xclip(int x, int y, nglBitmap bitmap) {}

void build_font_index(void) {}

void draw_text(int x, int y, uint8_t flags, const char* text) {

    const int font_size = 10;
    int xoffs = x;

    if (flags & TEXT_CENTRE || flags & TEXT_ALIGN_RIGHT) {
        int xlen = MeasureText(text, font_size);
        if (flags & TEXT_CENTRE) {
            xoffs -= xlen/2;
        } else {
            xoffs -= xlen;
        }
    }

    DrawText(text, xoffs, y, 10, flags&TEXT_INVERT ? BLACK : DCOL);
}

void draw_textf(int x, int y, uint8_t flags, const char *fmt, ...) {
    char str[64];
    va_list args;
    va_start (args, fmt);
    vsnprintf(str, sizeof(str), fmt, args);
    draw_text(x, y, flags, str);
    va_end(args);
}