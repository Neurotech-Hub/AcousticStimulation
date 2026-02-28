# Serial Command Speaker Control

This Arduino sketch enables real-time control of a VS1053B-based speaker output using serial commands. It generates continuous sine waves at given frequencies and amplitudes.

## Hardware Used
- **Microcontroller:** Adafruit Feather M4 Express (ATSAMD51)
- **Audio Module:** Music Maker FeatherWing w/ Amp (VS1053B)

## Usage

Connect the board via USB, open your Serial Monitor, and set the baud rate to **115200**. You can control the output with the following commands:

| Command Format | Example | Description |
|---|---|---|
| `<freq>,<amp>` | `400,80` | Plays a sine wave closest to 400 Hz at 80% volume. |
| `stop` | `stop` | Stops the current tone playback. |

### Parameters
- **Frequency (`freq`)**: Expected range is `20` to `5000` (Hz). 
  - *Note:* The VS1053 hardware sine test supports ~248 discrete frequencies. The code automatically calculates and plays the closest supported frequency to your input and prints the actual achieved frequency to the serial monitor.
- **Amplitude (`amp`)**: Expected range is `0` to `100` (%). 
  - `100` = Maximum volume (0 dB attenuation).
  - `0` = Total silence.
  - The volume mapping uses a practical range to ensure lower percentages remain audible through the amplifier.

### Live Updating
You do not need to send `stop` before changing parameters. If a tone is playing and you send a new `<freq>,<amp>` command (e.g., changing from `200,50` to `400,80`), the sketch internally resets the codec and seamlessly transitions to the new tone.
