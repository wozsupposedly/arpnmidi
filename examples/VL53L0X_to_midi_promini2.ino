//WOZ.LOL laser time of flight sensor to chromatic din midi note data
//for pro mini
//THIS VERSION DIDN'T WORK
#include <SPI.h>
#include <Wire.h>
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>
#include <VL53L0X.h>
VL53L0X sensor;
#define distfilter 8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(10, 2, NEO_GRB + NEO_KHZ800);
#define octave 4
#define chan 1
//#define MIDI_SERIAL_PORT Serial1

bool off;
int distchange = 0;
MIDI_CREATE_DEFAULT_INSTANCE();

void noteDIN(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOn(pitch, velocity, channel); //din midi starts at 1
}

void noteoffDIN(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOn(pitch, 0, channel); //din midi starts at 1
}

void setup() {
  Wire.begin();
  sensor.setTimeout(500);
  sensor.init();
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(200);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  sensor.setMeasurementTimingBudget(20000);
  sensor.startContinuous();

}

void loop() {
  vox();
}

void panic() {
  for (int p = 0; p < 34; p++) {
    MIDI.sendNoteOn((octave * 12) + p, 0, chan); //din midi starts at 1
  }
}

void ender() {
  if (!off) {
    off = true;
    panic();
    //  MIDI.sendNoteOn((octave * 12), 0, chan); //din midi starts at 1
    //  MIDI.sendNoteOn((octave * 12) + 27, 0, chan); //din midi starts at 1
    led();
  }
}

void vox() {
  int distcheck = sensor.readRangeContinuousMillimeters();  ////// get it
  if ((distcheck < 60) || (distcheck > 588)) {
    distcheck = 0;
    ender();
  }
  if ((distcheck < distchange - distfilter) || (distcheck > distchange + distfilter)) {
    off = false;
    int dist = ((distcheck - 60) / 4);
    if (dist > 132) {
      dist = 132;
    }
    if (dist < 0) {
      dist = 0;
    }
    int odist = ((distchange - 60) / 4);
    if (odist > 132) {
      odist = 132;
    }
    if (odist < 0) {
      odist = 0;
    }
    noteDIN(chan, (octave * 12) + (odist / 4), 0);
    if (odist - dist < 20) noteDIN(chan, (octave * 12) + (dist / 4), 64);
    //  noteOff(channel, odist / 8 + 71, 64);
    //  MidiUSB.flush();
    //  noteOn(channel, dist / 8 + 71, 64);
    //  MidiUSB.flush();
    distchange = distcheck;
    led();
  }

  if (sensor.timeoutOccurred()) {
    ender();
  }
}

void led() {
  int r;
  int g;
  int b;
  int cm = 127 - (distchange - 60) / 4;
  if (cm < 0) cm = 0;
  if (!off) {
    for (int d = 0; d < 10; d++) {
      r = cm * 4 - d * 16;
      g = cm * 3 - 112 - d * 16;
      b = cm * 2 - 196 - d * 16;
      if (r > 255)r = 255;
      if (r < 0)r = 0;
      if (g > 255)g = 255;
      if (g < 0)g = 0;
      if (b > 255)b = 255;
      if (b < 0)b = 0;
      strip.setPixelColor(9 - d, strip.Color(r, g, b));
    }
  } else {
    for (byte k = 0; k < 10; k++) {
      strip.setPixelColor(k, strip.Color(0, 0, 0));
    }
  }
  strip.show();
}
