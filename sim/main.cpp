#include "raylib.h"
#include "ngl.h"
#include "../src/input.h"
#include "../src/ui.hpp"
#include "../src/assets/assets.h"
#include "../src/audio.hpp"

extern "C" {
    void ngl_sim_begin(void);
    void ngl_sim_end(void);
    void audio_dma_callback(void);
    extern RenderTexture2D display;
    extern uint8_t led_value[NUM_LEDS];
}

const int display_scale = 4;
const int display_height = NGL_DISPLAY_HEIGHT * display_scale;
const int windowWidth = NGL_DISPLAY_WIDTH * display_scale;
const int windowHeight = display_height + 64;
Camera2D window_camera;


void render(void) {
    Rectangle sourceRec = {0.0f, 0.0f, (float)display.texture.width, -(float)display.texture.height};
    Rectangle destRec = {0, 0, NGL_DISPLAY_WIDTH*display_scale, display_height};
    Vector2 origin = {0.0f, 0.0f};
    window_camera.zoom = 1.0f;

    BeginDrawing();
    BeginMode2D(window_camera);
        // Display
        DrawTexturePro(display.texture, sourceRec, destRec, origin, 0.0f, WHITE);
    
        // LEDs
        for (int i=0; i<16; i++) {
            float ledcol = (128 + led_value[i]/2) / 256.0f;
            DrawRectangle((i%8)*32, display_height+(i/8)*32, 30,30, ColorFromHSV(0, 1.0f, ledcol));
        }

    EndMode2D();
    EndDrawing();    
}


int main(void) {
    InitWindow(windowWidth, windowHeight, "sim");

    ngl_init();
    ui_init();

    // Change asset data to point to raylib Texture2Ds instead of OLED-friendly bitmaps
    // This is a manual step at this stage
    Texture2D gauge_arc_tex = LoadTexture("../assets/gauge_arc.png");
    Texture2D gauge_test_tex = LoadTexture("../assets/gauge_test.png");
    gauge_arc.data = (const uint8_t*)&gauge_arc_tex; // There is a possible alignment issue here
    gauge_test.data = (const uint8_t*)&gauge_test_tex;

    
    while (!WindowShouldClose()) {
        ngl_sim_begin();

        audio_dma_callback();
        Input input = audio_wait();
        ui_process(input);
    
        // DrawText("Congrats! You created your first window!", 16, 16, 20, DCOL);
        // ngl_setpixel(0,0, 1);
        // ngl_rect(1,1, 32,32, 1);
        // ngl_rect(95,95, 32,32, 1);

        ngl_sim_end();
        render();
    }

    CloseWindow();
    return 0;
}
