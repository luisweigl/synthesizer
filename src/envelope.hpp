#pragma once

#include "voice.hpp"

class Envelope {
  private:
    float attack;
    float sustain;
    float decay;
    float release;

  public:
    Envelope();

    void setAttack(float attack);
    void setSustain(float sustain);
    void setDecay(float decay);
    void setRelease(float release);

    float getAmplitude(VoiceState state, int samples);
};