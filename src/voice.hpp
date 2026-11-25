#pragma once
#include "envelope.hpp"
#include "oscillator.hpp"
#include "voice_state.hpp"
#include <array>

#define OSCILLATORS 2

class Voice {
  private:
    VoiceState state;
    int samples; // number in samples in current phase
    std::array<float, OSCILLATORS> phases;
    float amplitude;
    float releaseStartAmp;
    int note;

  public:
    Voice();

    VoiceState getState() const;
    int getNote() const;
    void noteOn(int note);
    void noteOff();
    float getSample(const std::array<Oscillator, OSCILLATORS> &oscillators,
                    const Envelope &envelope);
};