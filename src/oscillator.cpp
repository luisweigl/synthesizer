#include "oscillator.hpp"
#include "waves.hpp"
#include <cmath>

Oscillator::Oscillator() : waveform(Waveform::Sine), octave(0), semitone(0), detune(0), level(0) {}

void Oscillator::setWaveform(Waveform waveform) { this->waveform = waveform; }

void Oscillator::setOctave(int octave) { this->octave = octave; }

void Oscillator::setSemitone(int semitone) { this->semitone = semitone; }

void Oscillator::setDetune(float detune) { this->detune = detune; }

void Oscillator::setLevel(float level) { this->level = level; }

float Oscillator::getFrequency(float baseFrequency) const {
    float shift = octave * 12.0f + semitone + detune / 100.0f;

    return baseFrequency * std::pow(2.0f, shift / 12.0f);
}

float Oscillator::getSample(float phase) const {
    switch (waveform) {
    case Waveform::Sine:
        return sine_wave(level, phase);
        break;
    case Waveform::Sawtooth:
        return saw_wave(level, phase);
        break;
    case Waveform::Square:
        return square_wave(level, phase);
        break;
    case Waveform::Triangle:
        return triangle_wave(level, phase);
        break;
    }

    return 0.0f;
}