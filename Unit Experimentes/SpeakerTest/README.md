# Speaker Test

This unit experiment contains a simple stereo speaker test configured for the **Adafruit Feather M4 Express** and the **Music Maker FeatherWing (VS1053B)**.

## Hardware Required
- Adafruit Feather M4 Express (ATSAMD51)
- Music Maker FeatherWing w/ Amp (VS1053B)
- 2x Speakers attached to the FeatherWing output blocks

## Description
The VS1053B chip on the Music Maker has a built-in **sine wave test mode**, which is used in this sketch to verify that both the left and right speakers are working properly, without even needing an SD card. 

The program cycles through three phases continuously:
1. **Both Channels**: Plays a range of frequencies (250Hz - 870Hz) across both channels.
2. **Left Channel Only**: Isolates the left channel, playing a 440Hz tone.
3. **Right Channel Only**: Isolates the right channel, playing a 440Hz tone.

## Setup Instructions
1. Stack the Music Maker FeatherWing onto the Feather M4 Express.
2. Connect your stereo speakers.
3. In the Arduino IDE, install the **Adafruit VS1053 Library**.
4. Select the **Adafruit Feather M4 Express** board in the tools menu.
5. Upload the `SpeakerTest.ino` sketch and open the Serial Monitor (using 115200 baud) to monitor output.

## Wiring Reference
Because the FeatherWing stacks directly over the Feather M4 Express, no custom wiring is needed. However, here are the default utilized pin mappings:
- **VS1053 CS**: Pin 6
- **VS1053 DCS**: Pin 10
- **VS1053 DREQ**: Pin 9
- **SD Card CS**: Pin 5 (Not utilized for this speaker test program)
