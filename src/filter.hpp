#pragma once

#include <algorithm>
#include <cmath>

enum FilterType {
    LowPass,
    HighPass,
    BandPass,
    Notch,
};

class Filter {
  private:
    FilterType type;
    float integrator1;
    float integrator2;
    float cutoff;
    float resonance;
    float sampleRate;
    float g;
    float q;
    float a1;
    float a2;
    float a3;

  public:
    Filter(float sampleRate);

    void setCutoff(float cutoff);
    void setResonance(float resonance);

    void updateCoefficients();

    float process(float input);
};
