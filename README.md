# Synthesizer

In this project, I try to build my own digital synthesizer.

## Software

This repository contains the C++ code for the full synthesizer. Only MIDI input and audio output libraries are used.
Currently, this codebase runs on Linux using the JACK audio API, but the goal is for it to run on a Teensy 4.1 Microcontroller.

## Hardware

Additionally, I want to build a complete, functioning synthesizer on which this code can be run.
The synthesizer should contain all necessary instruments to make cool sounds, that means multiple Oscillators, Filtering, Envelope Settings, etc.

Under `/PCB` is the KiCad project for the custom PCB that I want to design and build for the synthesizer.
The casing should later be 3D printed.

