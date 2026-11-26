#include "filter.hpp"

Filter::Filter(float sampleRate)
    : type(LowPass), cutoff(15000.0f), resonance(0.0f), integrator1(0.0f), integrator2(0.0f),
      g(0.0f), a1(1.0f), a2(1.0f), a3(1.0f), q(0.70710678f) {
    this->sampleRate = sampleRate;
    updateCoefficients();
}

void Filter::setCutoff(float cutoff) {
    this->cutoff = cutoff;
    updateCoefficients();
}

void Filter::setResonance(float resonance) {
    this->resonance = resonance;
    updateCoefficients();
}

void Filter::updateCoefficients() {
    q = 0.70710678f + std::clamp(resonance, 0.0f, 1.0f) * 7.29289322f;
    g = std::tan(M_PI * cutoff / sampleRate);
    a1 = 1.0f / (1.0f + g * (g + 1.0f / q));
    a2 = g * a1;
    a3 = g * a1;
}

float Filter::process(float input) {
    const float v3 = input - integrator2;
    const float v1 = a1 * integrator1 + a2 * v3;
    const float v2 = integrator2 + a2 * integrator1 + a3 * v3;

    integrator1 = 2.0f * v1 - integrator1;
    integrator2 = 2.0f * v2 - integrator2;

    float low = v2;
    float band = v1;
    float high = input - (1.0f / q) * band - low;
    float notch = input - (1.0f / q) * band;

    switch (type) {
    case LowPass:
        return low;
    case BandPass:
        return band;
    case HighPass:
        return high;
    case Notch:
        return notch;
    }

    return 0.0;
}