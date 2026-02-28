# Acoustic Stimulation

This repository contains firmware for generating and controlling acoustic tone stimulation using an Arduino-compatible microcontroller paired with a VS1053B audio codec and amplifier.

## Hardware Setup
- **Base Controller:** Adafruit Feather M4 Express (ATSAMD51)
- **Audio Output:** Adafruit Music Maker FeatherWing with Amplifier (VS1053B), stacked on top of the Feather.

## Directory Structure

- **[`SpeakerControl/`](./SpeakerControl/)**
  - The main interactive firmware module. It provides a real-time serial interface (115200 baud) allowing you to command the device to play specific sine wave frequencies at varying amplitudes seamlessly.
- **[`Unit Experimentes/`](./Unit Experimentes/)**
  - Contains earlier development unit tests and proof-of-concept sketches. 
  - For example, `SpeakerTest.ino` includes a hardcoded sequential tone cycle to verify both speaker channels are wired and functioning correctly out of the box.
