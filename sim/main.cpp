#include "raylib.h"
#include "ngl.h"
#include "../src/input.h"
#include "../src/ui.hpp"
#include "../src/assets/assets.h"

extern "C" {
    void ngl_sim_begin(void);
    void ngl_sim_end(void);
    void ngl_sim_write(void);
}

Input input_prev;


int main(void) {
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

        Input input = input_get();
        if (input_detect_events(&input, input_prev)) {
            input_prev = input;
        }
        ui_process(input);
    
        // DrawText("Congrats! You created your first window!", 16, 16, 20, DCOL);
        // ngl_setpixel(0,0, 1);
        // ngl_rect(1,1, 32,32, 1);
        // ngl_rect(95,95, 32,32, 1);

        ngl_sim_end();
        ngl_sim_write();
    }

    CloseWindow();
    return 0;
}
