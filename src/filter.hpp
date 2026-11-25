#pragma once

#include <algorithm>
#include <cmath>

class LowPassFilter {
  private:
    float integrator1 = 0.0f;
    float integrator2 = 0.0f;
    float g = 0.0f;
    float a1 = 1.0f;

  public:
    void setParameters(float cutoff, float resonance, float sampleRate) {
        const float q = 0.70710678f + std::clamp(resonance, 0.0f, 1.0f) * 7.29289322f;
        g = std::tan(M_PI * cutoff / sampleRate);
        a1 = 1.0f / (1.0f + g * (g + 1.0f / q));
    }

    float process(float input) {
        // Trapezoidal state-variable filter
        const float bandPass = a1 * (integrator1 + g * (input - integrator2));
        const float lowPass = integrator2 + g * bandPass;
        integrator1 = 2.0f * bandPass - integrator1;
        integrator2 = 2.0f * lowPass - integrator2;
        return lowPass;
    }
};
