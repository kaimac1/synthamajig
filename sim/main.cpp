#include "raylib.h"
#include "ngl.h"
#include "../src/input.h"
#include "../src/userinterface.hpp"
#include "../src/assets/assets.h"
#include "../src/audio.hpp"
#include "../src/synth_common.h"
#include <stdio.h>
#include <math.h>

extern "C" {
    void ngl_sim_begin(void);
    void ngl_sim_end(void);
    extern RenderTexture2D display;
    extern uint8_t led_value[NUM_LEDS];
}

const int display_scale = 4;
const int display_height = NGL_DISPLAY_HEIGHT * display_scale;
const int windowWidth = NGL_DISPLAY_WIDTH * display_scale;
const int windowHeight = display_height + 64;
Camera2D window_camera;

RawInput global_raw_input;

void raylib_audio_callback(void *buffer, unsigned int frames) {
    AudioBuffer abuf;
    abuf.samples = (uint16_t*)buffer;
    abuf.sample_count = frames;
    audio_callback(abuf, global_raw_input);
}


void render(void) {
    Rectangle sourceRec = {0.0f, 0.0f, (float)display.texture.width, -(float)display.texture.height};
    Rectangle destRec = {0, 0, NGL_DISPLAY_WIDTH*display_scale, display_height};
    Vector2 origin = {0.0f, 0.0f};
    window_camera.zoom = 1.0f;

    BeginDrawing();
    BeginMode2D(window_camera);
        // Display
        DrawTexturePro(display.texture, sourceRec, destRec, origin, 0.0f, WHITE);
        DrawRectangle(0, display_height, windowWidth,windowHeight-display_height, ColorFromHSV(180, 0.5f, 0.1f));
    
        // LEDs
        for (int i=0; i<16; i++) {
            float ledcol = logf(0.001f + led_value[i] / 256.0f);
            DrawRectangle((i%8)*32+2, display_height+(i/8)*32+2, 28,28, ColorFromHSV(0, 1.0f, ledcol));
        }

    EndMode2D();
    EndDrawing();
}


int main(void) {
    InitWindow(windowWidth, windowHeight, "sim");
    create_lookup_tables();
    ngl_init();

    UI ui;
    ui.init();

    // Change asset data to point to raylib Texture2Ds instead of OLED-friendly bitmaps
    // This is a manual step at this stage
    Texture2D gauge_arc_tex = LoadTexture("../assets/gauge_arc.png");
    Texture2D gauge_test_tex = LoadTexture("../assets/gauge_test.png");
    gauge_arc.data = (const uint8_t*)&gauge_arc_tex; // There is a possible alignment issue here
    gauge_test.data = (const uint8_t*)&gauge_test_tex;

    InitAudioDevice();
    AudioStream audio_stream = LoadAudioStream(44100, 16, 1); // 16 bit mono
    SetAudioStreamCallback(audio_stream, raylib_audio_callback);
    PlayAudioStream(audio_stream);
    
    while (!WindowShouldClose()) {
        ngl_sim_begin();

        // We can't read the inputs in the audio callback with raylib, so read it here
        global_raw_input = input_read();
        audio_wait();
        
        ui.process(global_raw_input);
    
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
