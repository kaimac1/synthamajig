#pragma once
#include "input.h"

RawInput audio_wait(void);
void audio_callback(AudioBuffer buffer, RawInput input);
