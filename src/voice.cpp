#include "voice.hpp"
#include "midi.hpp"

#define SAMPLE_RATE 48000

Voice::Voice()
    : state(Disabled), samples(0), phases(std::array<float, 2>{}), amplitude(0), releaseStartAmp(0),
      note(0) {}

VoiceState Voice::getState() const { return state; }

int Voice::getNote() const { return note; }

void Voice::noteOn(int note) {
    phases.fill(0.0f);
    this->note = note;
    this->samples = 0;
    this->state = Attack;
}

void Voice::noteOff() {
    releaseStartAmp = amplitude;
    state = Release;
    samples = 0;
}
float Voice::getSample(const std::array<Oscillator, OSCILLATORS> &oscillators,
                       const Envelope &envelope) {

    if (state == Disabled) {
        return 0.0f;
    }

    float sample = 0;

    for (int i = 0; i < oscillators.size(); i++) {
        sample += oscillators[i].getSample(phases[i]);

        float frequency = oscillators[i].getFrequency(midiToFreqeuncy(note));
        phases[i] += frequency / SAMPLE_RATE;
        phases[i] -= std::floor(phases[i]);
    }

    sample /= static_cast<float>(oscillators.size());

    if (state == Release) {
        const float duration = envelope.getRelease() * SAMPLE_RATE;

        amplitude =
            duration <= 0.0f ? 0.0f : releaseStartAmp * std::max(0.0f, 1.0f - samples / duration);
    } else {
        amplitude = envelope.getAmplitude(state, samples);
    }
    samples++;

    if (state == Attack && samples >= envelope.getAttack() * SAMPLE_RATE) {
        state = Decay;
        samples = 0;
    } else if (state == Decay && samples >= envelope.getDecay() * SAMPLE_RATE) {
        state = Sustain;
        samples = 0;
    } else if (state == Release && samples >= envelope.getRelease() * SAMPLE_RATE) {
        state = Disabled;
        samples = 0;
    }

    return amplitude * sample;
}