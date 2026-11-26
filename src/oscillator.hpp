#pragma once

enum Waveform { Sine, Triangle, Sawtooth, Square };

class Oscillator {
  private:
    Waveform waveform;
    float frequency;
    int octave;
    int semitone;
    float detune;
    float level;

  public:
    Oscillator();

    void setWaveform(Waveform waveform);
    void setOctave(int octave);
    void setSemitone(int semitone);
    void setDetune(float detune);
    void setLevel(float level);
    float getFrequency() const;
    void setFrequency(float frequency);
    void setPitch(float baseFrequency);

    float getSample(float phase) const;
};