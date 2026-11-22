#include "envelope.hpp"
#include <algorithm>

#define SAMPLE_RATE 48000

Envelope::Envelope() : attack(0.01), decay(0.1), sustain(0.5), release(0.01) {}

void Envelope::setAttack(float attack) { this->attack = attack; }

void Envelope::setDecay(float decay) { this->decay = decay; }

void Envelope::setSustain(float sustain) { this->sustain = sustain; }

void Envelope::setRelease(float release) { this->release = release; }

float Envelope::getAmplitude(VoiceState state, int samples) {

    switch (state) {
    case VoiceState::Attack: {
        float m = (1.0f / (SAMPLE_RATE * attack));
        return std::min(m * samples, 1.0f);
    }
    case VoiceState::Decay: {
        float m = (sustain - 1.0f) / (SAMPLE_RATE * decay);
        return std::max(1.0f + m * samples, sustain);
    }
    case VoiceState::Sustain: {
        return sustain;
    }
    case VoiceState::Release: {
        float m = (sustain / (SAMPLE_RATE * release));
        return std::max(sustain - m * samples, 0.0f);
    }
    }
}
