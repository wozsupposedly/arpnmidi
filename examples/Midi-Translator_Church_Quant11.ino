// UNCOMMENT LINE 143 the first time you upload this code, then put it back
// woz.lol Midi Cheater Transposer Quantizer with 128x64 oled
// this version outputs din and usb midi, with din midi in

#include <MIDI.h>
#include <SPI.h>
#include <Wire.h>
#include <Rotary.h>
#include <EEPROM.h>
#include <MIDIUSB.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Rotary r = Rotary(14, 15);
#define MIDI_SERIAL_PORT Serial1
#define MIDI_CC MIDI_CC_GENERAL1
#define ssdelay 5000000   // screen saver delay
#define push 16
#define b1 18
#define b2 19
#define b3 20
#define b4 21
#define b5 10
#define deblimit 12000
bool cw;
bool ccw;
bool unpush;  //true when knob is not pushed
byte btn;
bool guit;
int deb; // debounce
long ss; // screen saver time
bool saved;
bool loaded;
byte live;
bool pass;
byte chan;
bool keys[12];
char note[] = "C C#D D#E F F#G G#A A#B ";
byte major[7] = {0, 2, 4, 5, 7, 9, 11};
byte minor[7] = {0, 2, 3, 5, 7, 8, 10};
//byte hminor[7] = {0, 2, 3, 5, 7, 8, 11};
//byte mminor[7] = {0, 2, 3, 5, 7, 9, 11};
byte blues[7] = {0, 3, 5, 6, 7, 10, 0};
byte Mblues[7] = {0, 2, 3, 4, 7, 9, 0};
byte mode[2][4];

MIDI_CREATE_DEFAULT_INSTANCE();

void clean() {
  if (btn) {
    pass = false;
    for (int sc = 0; sc < 7; sc++) {
      if ((mode[1][btn - 1] == 0) || (mode[1][btn - 1] == 2)) {
        if ((live % 12) == ((major[sc] + mode[0][btn - 1]) % 12)) {
          pass = true;
        }
      }
      if ((mode[1][btn - 1] == 1) || (mode[1][btn - 1] == 2) || (mode[1][btn - 1] > 5)) {
        if ((live % 12) == (( ((mode[1][btn - 1] == 7) && sc == 5 ) + ((mode[1][btn - 1] > 5) && sc == 6 ) + minor[sc] + mode[0][btn - 1]) % 12)) {
          pass = true;
        }
      }
      if ((mode[1][btn - 1] == 3) || (mode[1][btn - 1] == 5)) {
        if ((live % 12) == ((blues[sc] + mode[0][btn - 1]) % 12)) {
          pass = true;
        }
      }
      if ((mode[1][btn - 1] == 4) || (mode[1][btn - 1] == 5)) {
        if ((live % 12) == ((Mblues[sc] + mode[0][btn - 1]) % 12)) {
          pass = true;
        }
      }
    }
    if (!pass) live++;
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {  // din midi in
  live = pitch;
  clean();
  clean();
  noteDIN(channel, live, velocity);
  noteUSB(channel - 1, live, velocity);
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {   // din midi in
  live = pitch;
  clean();
  clean();
  noteoffDIN(channel, live, velocity);
  noteoffUSB(channel - 1, live, velocity);
}

void panic(byte but) {
  for (byte t = 12; t < 96; t++) {
    MIDI.sendNoteOff(t, 0, chan + 1); //din midi starts at 1
  }
}

void handlepb(byte channel, int val)
{
  MIDI.sendPitchBend(val, channel);
  pitchBendChange(channel - 1, val);
}

void handlecc(byte channel, byte control, byte value) {    // din cc out
  MIDI.sendControlChange(control, value, channel);
  controlChange(channel - 1, control, value);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

void pitchBendChange(byte channel, int value) {
  byte lowValue = value & 0x7F;
  byte highValue = value >> 7;
  midiEventPacket_t event = {0x0E, 0xE0 | channel, lowValue, highValue};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

void noteUSB(byte channel, byte pitch, byte velocity) {   //usb midi channel starts at 0
  midiEventPacket_t on = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(on);
  MidiUSB.flush();
}

void noteoffUSB(byte channel, byte pitch, byte velocity) {   //usb midi channel starts at 0
  midiEventPacket_t off = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(off);
  MidiUSB.flush();
}

void noteDIN(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOn(pitch, velocity, channel); //din midi starts at 1
}
void noteoffDIN(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOff(pitch, velocity, channel); //din midi starts at 1
}

void setup() {
  delay(2000);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.clearDisplay();
  display.setTextColor(WHITE, BLACK);
  Wire.begin(); //Join the bus as master.
  Wire.setClock(3400000);

  pinMode(push, INPUT);
  pinMode(b1, INPUT_PULLUP);
  pinMode(b2, INPUT_PULLUP);
  pinMode(b3, INPUT_PULLUP);
  pinMode(b4, INPUT_PULLUP);
  pinMode(b5, INPUT_PULLUP);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handlecc);
  MIDI.setHandlePitchBend(handlepb);
  // !!! UNCOMMENT THE LINE BELOW ON FIRST RUN OF THIS CODE, then comment it again and upload this a second time
  //  save();

  load();  // load old setting from eeprom. this will fail if you don't save defaults above on first run
  disp();

}

void loop() {
  wheel();
  scan();
  // domidi(); //check usb midi
  MIDI.read(); //check din midi

  if (ss > ssdelay) {
    scrnsvr();
    ss = 0;
  }
  ss++;

}
void keyupdate() {
  for (int j = 0; j < 12; j++) {
    keys[j] = 0;
  }
  for (int sc = 0; sc < 7; sc++) {
    if ((mode[1][btn - 1] == 0) || (mode[1][btn - 1] == 2)) {
      keys[((major[sc] + mode[0][btn - 1]) % 12)] = true;
    }
    if ((mode[1][btn - 1] == 1) || (mode[1][btn - 1] == 2) || (mode[1][btn - 1] > 5)) {
      //((mode[1][btn - 1] == 7) && sc == 5 ) + ((mode[1][btn - 1] > 5) && sc == 6 ) + minor[sc] + mode[0][btn - 1]) % 12))
      keys[((((mode[1][btn - 1] == 7) && sc == 5 ) + ((mode[1][btn - 1] > 5) && sc == 6 ) + minor[sc] + mode[0][btn - 1]) % 12)] = true;
    }
    if ((mode[1][btn - 1] == 3) || (mode[1][btn - 1] == 5)) {
      keys[((blues[sc] + mode[0][btn - 1]) % 12)] = true;
    }
    if ((mode[1][btn - 1] == 4) || (mode[1][btn - 1] == 5)) {
      keys[((Mblues[sc] + mode[0][btn - 1]) % 12)] = true;
    }
  }
}

void gtr() {
  //strings
  for (int d = 0; d < 6; d++) {
    for (int e = 0; e < 128; e += 2) {
      display.drawPixel(e, 4 + (d * 8), WHITE);
    }
    // display.drawLine(0, 4 + (d * 8), 127, 4 + (d * 8), WHITE);
  }

  display.drawLine(7, 0, 7, 47, WHITE);
  for (int c = 0; c < 12; c++) {
    display.drawLine(8 + (10 * c), 0, 8 + (10 * c), 47, WHITE);
  }

  //markers
  display.drawCircle(33, 24, 1, WHITE);
  display.drawCircle(53, 24, 1, WHITE);
  display.drawCircle(73, 24, 1, WHITE);
  display.drawCircle(93, 24, 1, WHITE);
  display.drawCircle(123, 16, 1, WHITE);
  display.drawCircle(123, 32, 1, WHITE);

  for (int a = 0; a < 13; a++) {
    int ae = a + 4;
    if (ae > 11) {
      ae = ae - 12;
    }
    int aa = a + 9;
    if (aa > 11) {
      aa = aa - 12;
    }
    int ad = a + 2;
    if (ad > 11) {
      ad = ad - 12;
    }
    int ag = a + 7;
    if (ag > 11) {
      ag = ag - 12;
    }
    int ab = a + 11;
    if (ab > 11) {
      ab = ab - 12;
    }

    if (ae == mode[0][btn - 1]) {
      display.drawCircle(3 + (10 * a), 44, keys[ae] * 3, WHITE);
    } else {
      display.fillCircle(3 + (10 * a), 44, keys[ae] * 3, WHITE);
    }
    if (aa == mode[0][btn - 1]) {
      display.drawCircle(3 + (10 * a), 36, keys[aa] * 3, WHITE);
    } else {
      display.fillCircle(3 + (10 * a), 36, keys[aa] * 3, WHITE);
    }
    if (ad == mode[0][btn - 1]) {
      display.drawCircle(3 + (10 * a), 28, keys[ad] * 3, WHITE);
    }
    else {
      display.fillCircle(3 + (10 * a), 28, keys[ad] * 3, WHITE);
    }
    if (ag == mode[0][btn - 1]) {
      display.drawCircle(3 + (10 * a), 20, keys[ag] * 3, WHITE);
    } else {
      display.fillCircle(3 + (10 * a), 20, keys[ag] * 3, WHITE);
    }
    if (ab == mode[0][btn - 1]) {
      display.drawCircle(3 + (10 * a), 12, keys[ab] * 3, WHITE);
    } else {
      display.fillCircle(3 + (10 * a), 12, keys[ab] * 3, WHITE);
    }
    if (ae == mode[0][btn - 1]) {
      display.drawCircle(3 + (10 * a), 4, keys[ae] * 3, WHITE);
    } else {
      display.fillCircle(3 + (10 * a), 4, keys[ae] * 3, WHITE);
    }
  }
}

void drawkb() {
  if (keys[0]) {
    display.fillRoundRect(36, 16, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(36, 16, 7, 23, 3, WHITE);
  }
  if (keys[2]) {
    display.fillRoundRect(50, 16, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(50, 16, 7, 23, 3, WHITE);
  }
  if (keys[4]) {
    display.fillRoundRect(64, 16, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(64, 16, 7, 23, 3, WHITE);
  }
  if (keys[5]) {
    display.fillRoundRect(78, 16, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(78, 16, 7, 23, 3, WHITE);
  }
  if (keys[7]) {
    display.fillRoundRect(92, 16, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(92, 16, 7, 23, 3, WHITE);
  }
  if (keys[9]) {
    display.fillRoundRect(106, 16, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(106, 16, 7, 23, 3, WHITE);
  }
  if (keys[11]) {
    display.fillRoundRect(120, 16, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(120, 16, 7, 23, 3, WHITE);
  }
  if (keys[1]) {
    display.fillRoundRect(43, 0, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(43, 0, 7, 23, 3, WHITE);
  }
  if (keys[3]) {
    display.fillRoundRect(57, 0, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(57, 0, 7, 23, 3, WHITE);
  }
  if (keys[6]) {
    display.fillRoundRect(85, 0, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(85, 0, 7, 23, 3, WHITE);
  }
  if (keys[8]) {
    display.fillRoundRect(99, 0, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(99, 0, 7, 23, 3, WHITE);
  }
  if (keys[10]) {
    display.fillRoundRect(113, 0, 7, 23, 3, WHITE);
  } else {
    display.drawRoundRect(113, 0, 7, 23, 3, WHITE);
  }

}

void small(byte but, byte pos) {
  display.setCursor(pos, 56);
  if ((mode[1][but] > 2) && (mode[1][but] < 6)) display.print("B");
  if (mode[1][but] == 0 || mode[1][but] == 2 || ((mode[1][but] > 3) && (mode[1][but] < 6))) display.print("M");
  if (mode[1][but] == 6) display.print("H");
  if (mode[1][but] == 7) display.print("m");
  if (mode[1][but] == 1 || mode[1][but] == 2 || mode[1][but] > 4) display.print("m");
  int n = ((mode[0][but] % 12) * 2);
  display.setCursor(pos, 48);
  display.print(note[n]);
  display.print(note[n + 1]);
}

void disp() {
  display.clearDisplay();        // clear the internal memory
  byte p;
  for (byte b = 0; b < 4; b++) {
    p = 2 + (b * 32);
    small(b, p);
  }

  if (!btn) {
    display.setTextColor(BLACK);
    if (saved) {
      display.fillRect(0, 0, 127, 48, WHITE);
      display.setCursor(46, 20);
      display.print("SAVED");
      saved = false;
    } else if (loaded) {
      display.fillRect(42, 19, 37, 9, WHITE);
      display.setCursor(43, 20);
      display.print("LOADED");
      loaded = false;
    }
    display.setTextColor(WHITE, BLACK);
  } else {
    display.fillRect((btn - 1) * 32, 48, 32, 24, SSD1306_INVERSE);

    keyupdate();
    if (guit) {
      gtr();
    } else {
      bignote();
      drawkb();
    }
  }

  ss = 0;
  display.display();        // transfer internal memory to the display
}

void bignote() {
  if (mode[0][btn - 1] < 2) {               // C
    display.drawLine(0, 11, 0, 34, WHITE);
    display.drawLine(0, 11, 5, 0, WHITE);
    display.drawLine(10, 11, 5, 0, WHITE);
    display.drawLine(0, 34, 5, 45, WHITE);
    display.drawLine(10, 34, 5, 45, WHITE);
  }
  if ((mode[0][btn - 1] == 2) || (mode[0][btn - 1] == 3)) { // D
    display.drawLine(0, 0, 0, 45, WHITE);
    display.drawLine(0, 0, 10, 9, WHITE);
    display.drawLine(10, 36, 10, 9, WHITE);
    display.drawLine(10, 36, 0, 45, WHITE);
  }
  if (mode[0][btn - 1] == 4) { // E
    display.drawLine(0, 0, 0, 45, WHITE);
    display.drawLine(0, 0, 10, 0, WHITE);
    display.drawLine(0, 25, 8, 25, WHITE);
    display.drawLine(0, 45, 10, 45, WHITE);
  }
  if ((mode[0][btn - 1] == 5) || (mode[0][btn - 1] == 6)) { // F
    display.drawLine(0, 0, 0, 45, WHITE);
    display.drawLine(0, 0, 10, 0, WHITE);
    display.drawLine(0, 18, 8, 18, WHITE);
  }
  if ((mode[0][btn - 1] == 7) || (mode[0][btn - 1] == 8)) { // G
    display.drawLine(0, 11, 0, 34, WHITE);
    display.drawLine(0, 11, 5, 0, WHITE);
    display.drawLine(10, 11, 5, 0, WHITE);
    display.drawLine(0, 34, 5, 45, WHITE);
    display.drawLine(10, 34, 5, 45, WHITE);
    display.drawLine(10, 34, 10, 24, WHITE);
    display.drawLine(5, 24, 10, 24, WHITE);
  }
  if ((mode[0][btn - 1] == 9) || (mode[0][btn - 1] == 10)) { // A
    display.drawLine(0, 45, 5, 0, WHITE);
    display.drawLine(5, 0, 10, 45, WHITE);
    display.drawLine(3, 32, 7, 32, WHITE);
  }
  if (mode[0][btn - 1] == 11) {                     // B
    display.drawLine(0, 0, 0, 45, WHITE);
    display.drawLine(0, 0, 10, 10, WHITE);
    display.drawLine(10, 10, 0, 20, WHITE);
    display.drawLine(0, 20, 10, 30, WHITE);
    display.drawLine(0, 45, 10, 30, WHITE);
  }
  // #
  if ((mode[0][btn - 1] == 1) || (mode[0][btn - 1] == 3) || (mode[0][btn - 1] == 6) || (mode[0][btn - 1] == 8) || (mode[0][btn - 1] == 10)) {
    display.drawLine(12, 3, 22, 3, WHITE);
    display.drawLine(12, 7, 22, 7, WHITE);
    display.drawLine(15, 0, 15, 10, WHITE);
    display.drawLine(19, 0, 19, 10, WHITE);
  }
  // M
  if ((mode[1][btn - 1] == 4) || (mode[1][btn - 1] == 2) || (mode[1][btn - 1] == 5)) { // Major
    display.drawLine(13, 46, 13, 35, WHITE);
    display.drawLine(18, 39, 13, 35, WHITE);
    display.drawLine(18, 46, 18, 35, WHITE);
    display.drawLine(23, 39, 18, 35, WHITE);
    display.drawLine(23, 46, 23, 39, WHITE);
  }
  // m
  if ((mode[1][btn - 1] == 1) || (mode[1][btn - 1] == 2) || (mode[1][btn - 1] > 4)) { // minor
    for (int mi = 0; mi < 1 + (mode[1][btn - 1] == 7); mi++) {
      display.drawLine(13, 46 - (mi * 19), 13, 39 - (mi * 19), WHITE);
      display.drawLine(13, 42 - (mi * 19), 18, 39 - (mi * 19), WHITE);
      display.drawLine(18, 46 - (mi * 19), 18, 39 - (mi * 19), WHITE);
      display.drawLine(18, 42 - (mi * 19), 23, 39 - (mi * 19), WHITE);
      display.drawLine(23, 46 - (mi * 19), 23, 39 - (mi * 19), WHITE);
    }
  }
  // B
  if ((mode[1][btn - 1] > 2) && (mode[1][btn - 1] < 6)) { // blues
    display.drawLine(14, 17, 14, 29, WHITE);
    display.drawLine(14, 17, 21, 20, WHITE);
    display.drawLine(14, 23, 21, 20, WHITE);
    display.drawLine(14, 23, 21, 26, WHITE);
    display.drawLine(14, 29, 21, 26, WHITE);
  }
  // H
  if (mode[1][btn - 1] == 6) { // harmonic min
    display.drawLine(14, 17, 14, 29, WHITE);
    display.drawLine(21, 17, 21, 29, WHITE);
    display.drawLine(14, 23, 21, 23, WHITE);
  }
}

void scan() {
  byte bn;
  byte temp;
  unpush = digitalRead(push);
  if (deb > 0) {
    deb--;  //debounce
  } else {
    if (!digitalRead(b5)) {
      btn = 0;
      /*
            for (bn = 0; bn < 4; bn++) {
              if (!digitalRead(b1 + bn)) {
                // button 5 pressed at the same time as 1-4
                btn = bn + 1;
                disp();
                deb = deblimit;
                guit = false;
              }
            }
      */
      disp();
    } else {
      for (bn = 0; bn < 4; bn++) {
        if (!digitalRead(b1 + bn)) {
          if ((btn == bn + 1) && (ss != 1)) {
            if (guit) {
              guit = false;
            } else {
              guit = true;
            }
          } else {
            btn = bn + 1;
          }
          disp();
          deb = deblimit;
        }
      }
    }
  }

  if (cw) {
    cw = false;
    if (!btn) {
      if (!unpush) {
        save(); // if param == 2, we load
      }
    } else {
      panic(btn - 1);
      mode[unpush][btn - 1] ++;
      if (mode[0][btn - 1] > 11 || mode[1][btn - 1] > 7) mode[unpush][btn - 1] = 0;
    }
    disp();
  }  // end cw

  if (ccw) {
    ccw = false;
    if (!btn) {
      if (!unpush) {
        load(); // if param == 2, we load
      }
    } else {
      panic(btn - 1);
      mode[unpush][btn - 1] --;
      if (mode[0][btn - 1] > 11) mode[unpush][btn - 1] = 11;
      if (mode[1][btn - 1] > 7) mode[unpush][btn - 1] = 7;
    }

    disp();
  }  // end ccw
}


void domidi() {
  byte t;
  midiEventPacket_t rx = MidiUSB.read();
  switch (rx.header) {
    case 0:
      break; //No pending events
    case 0x9:
      //(rx.byte1 & 0xF) == chan
      //rx.byte2        //pitch
      //rx.byte3       //velocity
      break;
    case 0x8:
      //noteoff
      break;
  }
}

void scrnsvr() {
  long tim;
  int sy;
  int sx;
  int siz;
  int moon;
  ss = 1;
  while (!cw && !ccw && ss != 0) {
    wheel();
    scan();
    //   domidi();
    MIDI.read();
    if (!tim) {
      display.clearDisplay();        // clear the internal memory
      sy = random(18) + 30;
      sx = 0;
      siz = random(4) + 6;
      if (btn) {
        moon = random(16) + 8 + ((btn - 1) * 32);
      } else {
        moon = random(112) + 8;
      }
      while (sx < 127) {
        sx++;
        sy = sy + random(5) - 2;
        if (sy < siz + 4) {
          sy = siz + 4;
        }
        if (sy > 41) {
          sy = 41;
        }
        if (sx == moon)    display.fillCircle(sx, sy - random(8) - 8, siz, SSD1306_WHITE);
        display.drawPixel(sx, sy, SSD1306_WHITE);
      }
      sx = 0;
      if (btn) small(btn - 1, 2 + random(112));
      display.display();        // transfer internal memory to the display
    }
    tim++;
    // delay(1);
    if (tim > 100000) tim = 0;
  }
  disp();
}

void wheel() {
  unsigned char result = r.process();
  switch (result) {
    case DIR_CW:
      cw = true;
      break;
    case DIR_CCW:
      ccw = true;
      break;
  }
}

void load() {
  for (byte m = 0; m < 4; m++) {
    EEPROM.get(2 * m, mode[0][m]);
    EEPROM.get(8 + 2 * m, mode[1][m]);
  }
  loaded = true;
}

void save() {
  for (byte m = 0; m < 4; m++) {
    EEPROM.put(2 * m, mode[0][m]);
    EEPROM.put(8 + 2 * m, mode[1][m]);
  }
  saved = true;
}
