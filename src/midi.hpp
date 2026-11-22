#pragma once

#include <cmath>

#define TUNING 440.0

float midiToFreqeuncy(int note) { return TUNING * pow(2.0, (note - 69.0) / 12.0); }