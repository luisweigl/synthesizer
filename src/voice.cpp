#include "voice.hpp"
#include "midi.hpp"

#define SAMPLE_RATE 48000

float Voice::getPhase() const {
    double cycles = samples * midiToFreqeuncy(note) / SAMPLE_RATE;
    return cycles - std::floor(cycles);
}

float Voice::getSample(std::array<Oscillator, OSCILLATORS> oscillators, Envelope &envelope) {

    float sample = 0;

    for (auto &oscillator : oscillators) {
        sample += oscillator.getSample(getPhase());
    }

    sample /= static_cast<float>(oscillators.size());

    return envelope.getAmplitude(state, samples) * sample;
}