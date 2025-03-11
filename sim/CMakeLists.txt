cmake_minimum_required(VERSION 3.24...3.30)
project(sim)

link_directories(${CMAKE_CURRENT_LIST_DIR}/lib)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../src)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../src/gfx)

add_executable(${PROJECT_NAME}
    main.cpp
    sim_gfx.c
    sim_hw.c
    sim_perf.c

    ../src/input.c
    ../src/audio.cpp
    ../src/track.cpp
    ../src/userinterface.cpp
    ../src/instrument.cpp
    ../src/synth_common.cpp
    ../src/keyboard.c
    ../src/gfx/kmgui.c
    ../src/gfx/gfx_ext.c
    ../src/assets/assets.c
)

target_link_libraries(${PROJECT_NAME} raylib)