/*
 * AcousticStimulation - Stereo Speaker Test
 * 
 * Hardware:
 *   - Adafruit Feather M4 Express (ATSAMD51)
 *   - Music Maker FeatherWing w/ Amp (VS1053B)
 * 
 * This sketch uses the VS1053B's built-in sine wave test to verify
 * both speakers are working. It cycles through several frequencies
 * so you can hear distinct tones from each channel.
 * 
 * Wiring (FeatherWing stacks directly onto Feather):
 *   VS1053 CS   = Pin 6
 *   VS1053 DCS  = Pin 10
 *   VS1053 DREQ = Pin 9
 *   SD Card CS   = Pin 5  (not used yet)
 */

 #include <SPI.h>
 #include <Adafruit_VS1053.h>
 
 // ── Pin definitions (Feather M4 + Music Maker FeatherWing) ──
 #define VS1053_CS     6    // VS1053 chip select
 #define VS1053_DCS   10    // VS1053 data/command select
 #define VS1053_DREQ   9    // VS1053 data request (interrupt pin)
 #define CARDCS         5    // SD card chip select (for future use)
 
 // Create the Music Maker shield object
 Adafruit_VS1053_FilePlayer musicPlayer =
   Adafruit_VS1053_FilePlayer(VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);
 
 // ── Sine test frequency table ──
 struct ToneEntry {
   uint8_t regValue;
   uint16_t freqHz;
   const char* label;
 };
 
 const ToneEntry testTones[] = {
   { 0x44, 440,  "A4 (440 Hz)"  },
   { 0x58, 600,  "~600 Hz"      },
   { 0x6C, 740,  "~740 Hz"      },
   { 0x7E, 870,  "~870 Hz"      },
   { 0x24, 250,  "~250 Hz"      },
 };
 const int NUM_TONES = sizeof(testTones) / sizeof(testTones[0]);
 
 // ── Timing ──
 const unsigned long TONE_DURATION_MS = 2000;  // How long each tone plays
 const unsigned long PAUSE_BETWEEN_MS = 500;   // Silence between tones
 
 // ── Volume ──
 // VS1053 volume: 0 = loudest, 255 = silence
 // Set left and right independently to test stereo
 const uint8_t VOLUME_LEFT  = 20;
 const uint8_t VOLUME_RIGHT = 20;
 
 void setup() {
   Serial.begin(115200);
   
   // Wait up to 3 seconds for Serial Monitor
   unsigned long start = millis();
   while (!Serial && (millis() - start < 3000));
 
   Serial.println(F("========================================"));
   Serial.println(F(" Acoustic Stimulation - Speaker Test"));
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
   musicPlayer.setVolume(VOLUME_LEFT, VOLUME_RIGHT);
 }
 
 void playTone(const ToneEntry& tone, unsigned long durationMs) {
   Serial.print(F("  Playing: "));
   Serial.print(tone.label);
   Serial.print(F(" for "));
   Serial.print(durationMs);
   Serial.println(F(" ms"));
 
   musicPlayer.sineTest(tone.regValue, durationMs);
 }
 
 void loop() {
   // Phase 1: BOTH channels
   Serial.println(F("--- BOTH CHANNELS ---"));
   for (int i = 0; i < NUM_TONES; i++) {
     playTone(testTones[i], TONE_DURATION_MS);
     delay(PAUSE_BETWEEN_MS);
   }
 
   // Phase 2: LEFT channel
   Serial.println(F("--- LEFT CHANNEL ONLY ---"));
   musicPlayer.setVolume(VOLUME_LEFT, 255);  // Mute right
   delay(200);
   playTone(testTones[0], TONE_DURATION_MS);
   delay(PAUSE_BETWEEN_MS);
 
   // Phase 3: RIGHT channel
   Serial.println(F("--- RIGHT CHANNEL ONLY ---"));
   musicPlayer.setVolume(255, VOLUME_RIGHT);  // Mute left
   delay(200);
   playTone(testTones[0], TONE_DURATION_MS);
   delay(PAUSE_BETWEEN_MS);
 
   // Restore stereo volume
   musicPlayer.setVolume(VOLUME_LEFT, VOLUME_RIGHT);
 
   Serial.println();
   Serial.println(F("=== Test cycle complete. Restarting in 3s... ==="));
   Serial.println();
   delay(3000);
 }