#include "lfo.hpp"
#include <cmath>

LFO::LFO(float sampleRate) : value(0.0f), depth(0.0f) {
    this->sampleRate = sampleRate;
    this->oscillator = Oscillator();
    oscillator.setFrequency(0.0f);
}

void LFO::setRate(float rate) { this->oscillator.setFrequency(rate); }

float LFO::getValue() { return value; }

void LFO::process() {
    phase += oscillator.getFrequency() / sampleRate;
    phase -= std::floor(phase);

    value = oscillator.getSample(phase);
}
