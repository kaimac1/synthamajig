#pragma once
#include "input.h"

Input audio_wait(void);
void audio_callback(AudioBuffer buffer, Input input);
