#include "RtAudio.h"
#include "RtMidi.h"
#include "SPSCQueue.h"
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
    float attack;
    float release;
    float amplitude;
    float cutoff;
    float filteredSample;
};

void cleanup(RtAudio &audio) {

    if (audio.isStreamRunning())
        audio.stopStream();

    if (audio.isStreamOpen())
        audio.closeStream();
}

int selectOutputDevice(RtAudio &audio) {
    std::vector<unsigned int> deviceIds = audio.getDeviceIds();

    RtAudio::DeviceInfo info;

    std::cout << "Avilable devices:" << std::endl;

    for (auto id : deviceIds) {
        info = audio.getDeviceInfo(id);
        std::cout << "[" << id << "]" << info.name << std::endl;
    }
    std::cout << "Select a device: ";

    std::string input;
    std::getline(std::cin, input);

    std::size_t pos;
    int value = std::stol(input, &pos);

    if (input[pos] != '\0') {
        std::cout << "Invalid input!" << std::endl;
        cleanup(audio);
        exit(EXIT_FAILURE);
    }

    if (find(deviceIds.begin(), deviceIds.end(), value) == deviceIds.end()) {
        std::cout << "Invalid device id!" << std::endl;
    }

    info = audio.getDeviceInfo(value);
    std::cout << "Now playing on device " << info.name << std::endl;

    return value;
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

    if (status)
        std::cout << "Stream underflow detected!" << std::endl;

    while (!midiQueue->empty()) {

        MidiEvent *event = midiQueue->front();

        if (event->type == MidiEventType::NoteOn) {
            for (Voice &voice : *voices) {
                if (voice.state == VoiceState::Mute) {
                    voice.state = VoiceState::Attack;
                    voice.playtime = 0;
                    voice.amplitude = 0;
                    voice.targetAmp = 0.2;
                    voice.note = event->note;
                    break;
                }
            }
        } else if (event->type == MidiEventType::NoteOff) {
            for (Voice &voice : *voices) {
                if (voice.state != VoiceState::Mute && voice.state != VoiceState::Release &&
                    voice.note == event->note) {
                    voice.state = VoiceState::Release;
                    break;
                }
            }
        } else {
            float normalized = event->value / 127.0f;
            if (event->controller == 14) {
                state->cutoff = normalized * 5000.0f;
            } else if (event->controller == 15) {
                state->release = 0.01f + normalized * 1.99f;
            } else if (event->controller == 16) {
                state->amplitude = 0.0f + normalized * 1.0f;

            } else if (event->controller == 17) {
                if (normalized > 0.5) {
                    state->waveType = WaveType::Saw;
                } else {
                    state->waveType = WaveType::Sin;
                }
            }
        }

        midiQueue->pop();
    }

    float delta = 1.0 / SAMPLE_RATE;

    for (unsigned int i = 0; i < nBufferFrames; i++) {
        float sample = 0.0;

        for (Voice &voice : *voices) {

            if (voice.state == VoiceState::Mute) {
                continue;
            }

            if (voice.state == VoiceState::Attack) {
                float increment = (voice.targetAmp / state->attack) * delta;
                voice.amplitude = std::min(voice.amplitude + increment, voice.targetAmp);

                if (voice.amplitude == voice.targetAmp) {
                    voice.state = VoiceState::Playing;
                }

            } else if (voice.state == VoiceState::Release) {
                float decrement = (voice.targetAmp / state->release) * delta;
                voice.amplitude = std::max(voice.amplitude - decrement, 0.0f);

                if (voice.amplitude == 0) {
                    voice.state = VoiceState::Mute;
                }
            }

            float frequency = midiToFreqeuncy(voice.note);
            if (state->waveType == WaveType::Sin) {
                sample += sine_wave(voice.amplitude, frequency, voice.playtime);
            } else {
                sample += saw_wave(voice.amplitude, frequency, voice.playtime);
            }
            voice.playtime += delta;
        }

        sample *= state->amplitude;

        sample = std::clamp(sample, -1.0f, 1.0f);

        float alpha = 1.0f - std::exp(-2.0f * 3.14159265f * state->cutoff / SAMPLE_RATE);

        state->filteredSample += alpha * (sample - state->filteredSample);

        sample = state->filteredSample;

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
    parameters.deviceId = audio.getDefaultOutputDevice();
    parameters.nChannels = 2;

    unsigned int sampleRate = SAMPLE_RATE;
    unsigned int bufferFrames = BUFFER_FRAMES;

    std::array<Oscillator, 2> oscillators{};

    std::array<Voice, VOICES> voices{};

    for (Voice &voice : voices) {
        voice.state = VoiceState::Mute;
    }

    rigtorp::SPSCQueue<MidiEvent> midiQueue(100);

    State state;
    state.midiQueue = &midiQueue;
    state.voices = &voices;
    state.attack = 0.01;
    state.release = 0.01;
    state.cutoff = 20000.0;
    state.amplitude = 0.5f;
    state.filteredSample = 0.0f;
    state.waveType = WaveType::Sin;

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
