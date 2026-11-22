#pragma once

enum Waveform { Sine, Triangle, Sawtooth, Square };

class Oscillator {
  private:
    Waveform waveform;
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
    float getFrequency(float baseFrequency) const;

    float getSample(float phase) const;
};