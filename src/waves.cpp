#include "waves.hpp"
#include <cmath>

float sine_wave(float amplitude, float phase) { return amplitude * std::sin(2.0f * M_PI * phase); }

float saw_wave(float amplitude, float phase) {
    phase -= std::floor(phase);
    return amplitude * (2.0f * phase - 1.0f);
}

float triangle_wave(float amplitude, float phase) {
    phase -= std::floor(phase);
    return amplitude * (1.0f - 4.0f * std::abs(phase - 0.5f));
}

float square_wave(float amplitude, float phase) {
    phase -= std::floor(phase);
    return amplitude * (phase < 0.5f ? 1.0f : -1.0f);
}