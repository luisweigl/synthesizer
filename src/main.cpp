#include "RtAudio.h"
#include "RtMidi.h"
#include "SPSCQueue.h"
#include "filter.hpp"
#include "lfo.hpp"
#include "oscillator.hpp"
#include "voice.hpp"
#include "waves.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <queue>
#include <vector>

#define TUNING 440.0
#define SAMPLE_RATE 48000
#define BUFFER_FRAMES 256
#define VOICES 8
#define OSCILLATORS 2

enum MidiEventType { NoteOn, NoteOff, ControlChange };

struct MidiEvent {
    MidiEventType type;

    // NoteOn or NoteOff
    int note = 0;
    int velocity = 0;

    // ControlChange
    int controller = 0;
    int value = 0;
};

struct State {
    rigtorp::SPSCQueue<MidiEvent> *midiQueue;
    std::array<Voice, VOICES> *voices;
    std::array<Oscillator, OSCILLATORS> *oscillators;
    Envelope *envelope;
    Filter *filter;
    LFO *lfo;
    float amplitude;
};

void cleanup(RtAudio &audio) {

    if (audio.isStreamRunning())
        audio.stopStream();

    if (audio.isStreamOpen())
        audio.closeStream();
}

int selectOutputDevice(RtAudio &audio) {
    std::vector<unsigned int> deviceIds = audio.getDeviceIds();

    std::vector<unsigned int> outputIds;
    std::cout << "Available stereo output devices:" << std::endl;

    for (auto id : deviceIds) {
        const auto info = audio.getDeviceInfo(id);
        if (info.outputChannels < 2)
            continue;
        outputIds.push_back(id);
        std::cout << "[" << id << "] " << info.name << std::endl;
    }
    if (outputIds.empty()) {
        std::cerr << "No stereo output devices available.\n";
        exit(EXIT_FAILURE);
    }

    while (true) {
        std::cout << "Select an output device: ";
        std::string input;
        if (!std::getline(std::cin, input))
            exit(EXIT_FAILURE);

        try {
            std::size_t pos;
            const auto value = std::stoul(input, &pos);
            if (pos == input.size() &&
                std::find(outputIds.begin(), outputIds.end(), value) != outputIds.end()) {
                std::cout << "Audio output: " << audio.getDeviceInfo(value).name << std::endl;
                return static_cast<int>(value);
            }
        } catch (const std::invalid_argument &) {
        } catch (const std::out_of_range &) {
        }
        std::cout << "Invalid output device id.\n";
    }
}

int selectMidiDevice(RtMidiIn &midi) {
    auto nPorts = midi.getPortCount();

    std::cout << nPorts << " available Midi devices" << std::endl;
    std::cout << "Select a port: ";

    std::string input;
    std::getline(std::cin, input);

    std::size_t pos;
    int value = std::stol(input, &pos);

    if (input[pos] != '\0') {
        std::cout << "Invalid input!" << std::endl;
        exit(EXIT_FAILURE);
    }

    return value;
}

int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                  double streamTime, RtAudioStreamStatus status, void *userData) {

    float *output = static_cast<float *>(outputBuffer);

    State *state = static_cast<State *>(userData);

    auto midiQueue = state->midiQueue;
    auto voices = state->voices;
    auto oscillators = state->oscillators;
    auto envelope = state->envelope;
    auto filter = state->filter;

    if (status)
        std::cout << "Stream underflow detected!" << std::endl;

    while (!midiQueue->empty()) {

        MidiEvent &event = *midiQueue->front();

        if (event.type == NoteOn) {
            for (Voice &voice : *voices) {
                if (voice.getState() == Disabled) {
                    voice.noteOn(event.note);
                    break;
                }
            }
        } else if (event.type == NoteOff) {
            for (Voice &voice : *voices) {
                if (voice.getState() != Disabled && voice.getState() != Release &&
                    voice.getNote() == event.note) {
                    voice.noteOff();
                    break;
                }
            }
        } else if (event.type == ControlChange) {
            const float normalized = std::clamp(event.value, 0, 127) / 127.0f;
            switch (event.controller) {
            case 14:
                // Exponential sweep gives the low frequencies more knob travel.
                filter->setCutoff(20.0f * std::pow(1000.0f, normalized));
                break;
            case 15:
                filter->setResonance(normalized);
                break;
            case 16: {
                // Oscillator 1 stays in tune; oscillator 2 moves +/-25 cents.
                const int value = std::clamp(event.value, 0, 127);
                const float detune =
                    value <= 64 ? (value - 64) * (25.0f / 64.0f) : (value - 64) * (25.0f / 63.0f);
                (*oscillators)[1].setDetune(detune);
                break;
            }
            case 17:
                envelope->setRelease(0.01f * std::pow(300.0f, normalized));
                break;
            }
        }

        midiQueue->pop();
    }

    for (unsigned int i = 0; i < nBufferFrames; i++) {
        float sample = 0.0;

        for (Voice &voice : *voices) {

            sample += voice.getSample(*oscillators, *envelope);
        }

        sample *= state->amplitude;

        sample = filter->process(sample);
        sample = std::clamp(sample, -1.0f, 1.0f);

        output[2 * i] = sample;
        output[2 * i + 1] = sample;
    }
    return 0;
}

void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData) {

    auto *queue = static_cast<rigtorp::SPSCQueue<MidiEvent> *>(userData);

    int status = (*message)[0];
    int note = (*message)[1];
    int velocity = (*message)[2];

    std::cout << status << " " << note << " " << velocity << std::endl;

    if (status == 144 || status == 128 || status == 176) {
        MidiEvent event;

        switch (status) {
        case 144:
            event.type = MidiEventType::NoteOn;
            event.note = note;
            event.velocity = velocity;
            break;
        case 128:
            event.type = MidiEventType::NoteOff;
            event.note = note;
            event.velocity = velocity;
            break;
        case 176:
            event.type = MidiEventType::ControlChange;
            event.controller = note;
            event.value = velocity;
        }

        queue->try_push(event);
    }
}

int main(int, char **) {

    RtAudio audio;

    std::unique_ptr<RtMidiIn> midi;

    RtAudio::StreamParameters parameters;
    parameters.deviceId = selectOutputDevice(audio);
    parameters.nChannels = 2;

    unsigned int sampleRate = SAMPLE_RATE;
    unsigned int bufferFrames = BUFFER_FRAMES;

    Envelope envelope;
    envelope.setRelease(0.3f);

    std::array<Oscillator, 2> oscillators{};

    for (Oscillator &oscillator : oscillators) {
        oscillator.setLevel(0.2f);
        oscillator.setWaveform(Sawtooth);
    }
    oscillators[1].setDetune(7.0f);

    Filter filter(SAMPLE_RATE);
    LFO lfo(SAMPLE_RATE);

    std::array<Voice, VOICES> voices{};

    rigtorp::SPSCQueue<MidiEvent> midiQueue(100);

    State state{};
    state.midiQueue = &midiQueue;
    state.oscillators = &oscillators;
    state.envelope = &envelope;
    state.voices = &voices;
    state.amplitude = 0.5f;
    state.filter = &filter;
    state.lfo = &lfo;

    if (audio.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames,
                         &audioCallback, &state, NULL) != 0) {
        std::cout << audio.getErrorText() << std::endl;
        cleanup(audio);
        exit(EXIT_FAILURE);
    }

    try {
        midi = std::make_unique<RtMidiIn>();
    } catch (RtMidiError error) {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    int midiPort = selectMidiDevice(*midi);

    midi->openPort(midiPort);

    midi->setCallback(&midiCallback, &midiQueue);

    if (audio.startStream()) {
        std::cout << audio.getErrorText() << std::endl;
        cleanup(audio);
    }

    std::cout << "\nPlaying ... press <Enter> to quit.\n";
    std::getchar();
    cleanup(audio);

    exit(EXIT_SUCCESS);
}
