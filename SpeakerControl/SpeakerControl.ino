/*
 * AcousticStimulation - Serial Command Speaker Control
 * 
 * Hardware:
 *   - Adafruit Feather M4 Express (ATSAMD51)
 *   - Music Maker FeatherWing w/ Amp (VS1053B)
 * 
 * Serial Commands (115200 baud):
 *   <freq>,<amp>   e.g. "200,50"  → Play sine at ~200 Hz, 50% amplitude
 *   stop                          → Stop the current tone
 * 
 * Frequency range : 20 – 5000 Hz (mapped to closest available VS1053 freq)
 * Amplitude range : 0 – 100 %
 * 
 * Uses the VS1053B built-in sine test via SDI interface (same proven method
 * as SpeakerTest.ino) with non-blocking start/stop control.
 * 
 * Note: VS1053 sine test supports ~248 discrete frequencies. The sketch
 * automatically finds the closest match and reports the actual frequency.
 * 
 * Wiring (FeatherWing stacks directly onto Feather):
 *   VS1053 CS   = Pin 6
 *   VS1053 DCS  = Pin 10
 *   VS1053 DREQ = Pin 9
 *   SD Card CS  = Pin 5  (not used yet)
 */

#include <SPI.h>
#include <Adafruit_VS1053.h>

// ── Pin definitions (Feather M4 + Music Maker FeatherWing) ──
#define VS1053_CS     6
#define VS1053_DCS   10
#define VS1053_DREQ   9
#define CARDCS         5

// ── VS1053 register / bit constants ──
#define VS1053_REG_MODE  0x00
#define SM_TESTS         0x0020   // Test mode enable bit
#define SM_RESET         0x0004   // Software reset bit

// ── SPI setting for VS1053 data writes ──
#define VS1053_DATA_SPI  SPISettings(8000000, MSBFIRST, SPI_MODE0)

// ── Configuration ──
#define FREQ_MIN   20
#define FREQ_MAX   5000
#define AMP_MIN    0
#define AMP_MAX    100

// ── Sine test sample rate table (indexed by FsIdx, bits 7:5 of regValue) ──
// Formula: F = sampleRates[FsIdx] * S / 128, where S = bits 4:0
const uint16_t sampleRates[] = {44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000};

// ── State ──
bool     isPlaying     = false;
uint16_t currentFreq   = 0;
uint16_t actualFreq    = 0;
uint8_t  currentAmp    = 0;
uint8_t  currentRegN   = 0;

// Create the Music Maker shield object
Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

// ── Forward declarations ──
void processCommand(String cmd);
uint8_t  freqToRegValue(uint16_t targetFreq, uint16_t &actualFreqOut);
uint16_t regValueToFreq(uint8_t n);
void sineStart(uint8_t n, uint8_t amplitudePct);
void sineStop();
void setVolume(uint8_t amplitudePct);
void printHelp();

// ═══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  // Wait up to 3 seconds for Serial Monitor
  unsigned long start = millis();
  while (!Serial && (millis() - start < 3000));

  Serial.println(F("========================================"));
  Serial.println(F(" Acoustic Stimulation - Speaker Control"));
  Serial.println(F(" Feather M4 + Music Maker FeatherWing"));
  Serial.println(F("========================================"));

  if (!musicPlayer.begin()) {
    Serial.println(F("ERROR: VS1053 not found! Check wiring."));
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);
      delay(200);
    }
  }

  Serial.println(F("VS1053 found and initialized."));
  printHelp();
  Serial.println(F("Ready. Waiting for commands..."));
  Serial.println();
}

// ═══════════════════════════════════════════════════════════════
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      processCommand(input);
    }
  }
}

// ═══════════════════════════════════════════════════════════════
//  Command Processing
// ═══════════════════════════════════════════════════════════════

void processCommand(String cmd) {
  // ── "stop" command ──
  if (cmd.equalsIgnoreCase("stop")) {
    if (isPlaying) {
      sineStop();
      isPlaying   = false;
      currentFreq = 0;
      actualFreq  = 0;
      currentAmp  = 0;
      currentRegN = 0;
      Serial.println(F("OK: Stopped"));
    } else {
      Serial.println(F("OK: Already stopped"));
    }
    return;
  }

  // ── "freq,amp" command ──
  int commaIdx = cmd.indexOf(',');
  if (commaIdx < 0) {
    Serial.println(F("ERROR: Invalid format. Use <freq>,<amp> or 'stop'"));
    return;
  }

  String freqStr = cmd.substring(0, commaIdx);
  String ampStr  = cmd.substring(commaIdx + 1);
  freqStr.trim();
  ampStr.trim();

  long freq = freqStr.toInt();
  long amp  = ampStr.toInt();

  // toInt() returns 0 for non-numeric; disambiguate "0" from bad input
  if (freq == 0 && freqStr != "0") {
    Serial.println(F("ERROR: Invalid frequency value"));
    return;
  }
  if (amp == 0 && ampStr != "0") {
    Serial.println(F("ERROR: Invalid amplitude value"));
    return;
  }

  // Range checks
  if (freq < FREQ_MIN || freq > FREQ_MAX) {
    Serial.print(F("ERROR: Frequency must be "));
    Serial.print(FREQ_MIN);
    Serial.print(F("-"));
    Serial.print(FREQ_MAX);
    Serial.println(F(" Hz"));
    return;
  }

  if (amp < AMP_MIN || amp > AMP_MAX) {
    Serial.print(F("ERROR: Amplitude must be "));
    Serial.print(AMP_MIN);
    Serial.print(F("-"));
    Serial.print(AMP_MAX);
    Serial.println(F("%"));
    return;
  }

  // ── Map frequency to closest available VS1053 register value ──
  uint16_t actualF = 0;
  uint8_t regN = freqToRegValue((uint16_t)freq, actualF);

  // ── If already playing, stop current tone first ──
  if (isPlaying) {
    sineStop();
  }

  // ── Start new tone (reset + volume + sine all handled inside) ──
  sineStart(regN, (uint8_t)amp);

  isPlaying   = true;
  currentFreq = (uint16_t)freq;
  actualFreq  = actualF;
  currentAmp  = (uint8_t)amp;
  currentRegN = regN;

  Serial.print(F("OK: Playing "));
  Serial.print(actualF);
  Serial.print(F(" Hz (requested "));
  Serial.print(freq);
  Serial.print(F(" Hz) at "));
  Serial.print(amp);
  Serial.println(F("%"));
}

// ═══════════════════════════════════════════════════════════════
//  Frequency ↔ Register Mapping
// ═══════════════════════════════════════════════════════════════

// Find the VS1053 register value (n) that produces the frequency
// closest to targetFreq. Also outputs the actual frequency.
uint8_t freqToRegValue(uint16_t targetFreq, uint16_t &actualFreqOut) {
  int32_t bestDiff = 999999;
  uint8_t bestN = 0x44;        // Default: A4 (440 Hz)
  uint16_t bestFreq = 440;

  for (uint8_t fsIdx = 0; fsIdx < 8; fsIdx++) {
    for (uint8_t s = 1; s <= 31; s++) {
      uint16_t f = (uint32_t)sampleRates[fsIdx] * s / 128;
      int32_t diff = abs((int32_t)f - (int32_t)targetFreq);
      if (diff < bestDiff) {
        bestDiff = diff;
        bestN = (fsIdx << 5) | s;
        bestFreq = f;
      }
    }
  }

  actualFreqOut = bestFreq;
  return bestN;
}

// Convert register value back to frequency (for display)
uint16_t regValueToFreq(uint8_t n) {
  uint8_t fsIdx = (n >> 5) & 0x07;
  uint8_t s     = n & 0x1F;
  return (uint32_t)sampleRates[fsIdx] * s / 128;
}

// ═══════════════════════════════════════════════════════════════
//  VS1053 Sine Test – Non-blocking Start / Stop
//  (Same SDI method as sineTest() in the Adafruit library,
//   but split into separate start/stop for interactive use)
// ═══════════════════════════════════════════════════════════════

void sineStart(uint8_t n, uint8_t amplitudePct) {
  // 1) Full reset — puts VS1053 in a clean, known state.
  //    This is critical: without reset, the chip enters a bad state
  //    after stopping a sine test and won't accept new commands.
  //    (The Adafruit library's sineTest() does this same reset.)
  musicPlayer.reset();

  // 2) Set volume AFTER reset (reset clears all registers)
  setVolume(amplitudePct);

  // 3) Enable sine test mode (SM_TESTS bit in MODE register)
  uint16_t mode = musicPlayer.sciRead(VS1053_REG_MODE);
  mode |= SM_TESTS;
  musicPlayer.sciWrite(VS1053_REG_MODE, mode);

  // 4) Wait for VS1053 to be ready
  while (!digitalRead(VS1053_DREQ));

  // 5) Send sine-on command via SDI (data interface)
  //    Magic sequence: 0x53 0xEF 0x6E <n> 0x00 0x00 0x00 0x00
  uint8_t startCmd[] = {0x53, 0xEF, 0x6E, n, 0x00, 0x00, 0x00, 0x00};

  SPI.beginTransaction(VS1053_DATA_SPI);
  digitalWrite(VS1053_DCS, LOW);
  for (uint8_t i = 0; i < 8; i++) {
    SPI.transfer(startCmd[i]);
  }
  digitalWrite(VS1053_DCS, HIGH);
  SPI.endTransaction();
}

void sineStop() {
  // Wait for VS1053 to be ready
  while (!digitalRead(VS1053_DREQ));

  // Send sine-off command via SDI (data interface)
  // Magic sequence: 0x45 0x78 0x69 0x74 0x00 0x00 0x00 0x00
  uint8_t stopCmd[] = {0x45, 0x78, 0x69, 0x74, 0x00, 0x00, 0x00, 0x00};

  SPI.beginTransaction(VS1053_DATA_SPI);
  digitalWrite(VS1053_DCS, LOW);
  for (uint8_t i = 0; i < 8; i++) {
    SPI.transfer(stopCmd[i]);
  }
  digitalWrite(VS1053_DCS, HIGH);
  SPI.endTransaction();

  delay(10);

  // Clear SM_TESTS bit
  uint16_t mode = musicPlayer.sciRead(VS1053_REG_MODE);
  mode &= ~SM_TESTS;
  musicPlayer.sciWrite(VS1053_REG_MODE, mode);
}

// ═══════════════════════════════════════════════════════════════
//  Volume Control
// ═══════════════════════════════════════════════════════════════

void setVolume(uint8_t amplitudePct) {
  // VS1053 volume register: 0x00 = loudest, each step = 0.5 dB attenuation
  //
  // Practical range: 0 to 50 (0 to 25 dB attenuation)
  // Using 0-254 made anything below 100% inaudible through the amp.
  //
  // Mapping:
  //   100% → vol  0  (0.0 dB, loudest)
  //    50% → vol 25  (12.5 dB)
  //    10% → vol 45  (22.5 dB)
  //     0% → vol 0xFE (silence)
  uint8_t vol;
  if (amplitudePct >= 100) {
    vol = 0x00;
  } else if (amplitudePct == 0) {
    vol = 0xFE;  // True silence
  } else {
    vol = (uint8_t)(50 - ((uint16_t)amplitudePct * 50 / 100));
  }

  // Set both channels (stereo)
  musicPlayer.setVolume(vol, vol);
}

// ═══════════════════════════════════════════════════════════════
//  Help
// ═══════════════════════════════════════════════════════════════

void printHelp() {
  Serial.println();
  Serial.println(F("Commands:"));
  Serial.println(F("  <freq>,<amp>  Play sine wave (e.g. 200,50)"));
  Serial.print(  F("                freq: "));
  Serial.print(FREQ_MIN);
  Serial.print(F("-"));
  Serial.print(FREQ_MAX);
  Serial.println(F(" Hz"));
  Serial.print(  F("                amp:  "));
  Serial.print(AMP_MIN);
  Serial.print(F("-"));
  Serial.print(AMP_MAX);
  Serial.println(F("%"));
  Serial.println(F("  stop          Stop the current tone"));
  Serial.println();
  Serial.println(F("NOTE: Frequency is mapped to closest available"));
  Serial.println(F("      VS1053 sine test frequency (~248 steps)."));
  Serial.println();
}
