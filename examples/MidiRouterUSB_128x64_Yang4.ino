
#include <EEPROM.h>
//#include <MIDIUSB.h>
#include <MIDI.h>
#include <Rotary.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, 0);

Rotary r = Rotary(15, 14);
#define MIDI_SERIAL_PORT Serial1
#define MIDI_CC MIDI_CC_GENERAL1
MIDI_CREATE_DEFAULT_INSTANCE();
#define ssdelay 1000000   // screen saver delay
#define push 16
#define b1 10
#define deblimit 24000
#define damp 0 // sensitivity adjustment for touch force sensors. lower number is MORE sensitive

bool cw;
bool ccw;
byte param;
byte select;
int deb; // debounce
long ss; // screen saver time
bool saved;
bool loaded;
int tcheck1;
byte lastNote;
bool vDown;  // direction of vibrato
bool vState;  // active new instance of vibrato (resetting direction)
int vPos;  // current vibrato position
byte vWidth = 25; // vibrato distance setting
int vWidthFixed;  // vibrato setting fixed
int vDelay;

const byte vSpeedValue[] = { //Pitches (speed) for each note +36
  19,  20,  21,  22,  23,  24,  25,
  26, 27, 28, 29, 30, 32, 34, 37, 40,
  43, 45, 48, 51, 53, 56, 59, 61, 64,
  67, 69, 72, 75, 78, 80, 83, 85, 88,
  91, 93, 96, 97, 99, 100, 102, 103,
  104, 106, 107, 108, 109, 111, 112
}; // Note to pitch values for Korg Volca Sample 2015 by Mauricio Maisterrena

byte settings[5][16] {           //
  {13, 13, 13, 13, 13, 13, 13, 13,   0, 0, 0, 1, 14, 13, 13, 12},// out chan, 0=volca sample drum filter
  {0, 0, 0, 0, 0, 0, 0, 0,           1, 1, 1, 1, 1, 1, 1, 1},// mute (on?)
  {4, 4, 4, 4, 4, 4, 4, 4,           4, 4, 4, 4, 4, 4, 4, 4},// 0 1 2 3 (4) 5 6 7 8 octave shift, 4 is unchanged
  {0, 0, 0, 0, 0, 0, 0, 0,           1, 1, 1, 1, 0, 0, 0, 1},// volca bend mode
  {1, 1, 1, 1, 1, 1, 1, 1,           1, 1, 1, 1, 1, 1, 1, 1}// cc#
};

byte drum[3][6] {  // yamaha qy to volca sample
  { 36, 38, 40, 43, 37, 39},// note for part 1,2,3,4, 9,10 (5-8 for poly notes)
  { 1, 1, 1, 1, 1, 1},// mute (on?)
  { 1, 1, 1, 1, 1, 1} // cc#
};

void handleNoteOn(byte channel, byte pitch, byte velocity) {  // midi in
  if (settings[1][channel - 1]) {
    int pitchy = (settings[2][channel - 1] * 12) + pitch - 48;
    int channely = settings[0][channel - 1];
    if (!channely) {
      // drum filter
      for (byte vo = 0; vo < 6; vo++) {
        if (pitchy == drum[0][vo]) {
          pitchy = 36 + vo;
          if (vo > 3) pitchy = pitchy + 4;
          MIDI.sendNoteOn(pitchy, velocity, 11); //din midi starts at 1
          break;
        }
      }
    } else {
      MIDI.sendNoteOn(pitchy, velocity, channely); //din midi starts at 1
      //  midiEventPacket_t on = {0x09, 0x90 | 4, pitch, velocity};
      //  MidiUSB.sendMIDI(on);
      //  MidiUSB.flush();
      if ((channel - 1 == select) && (settings[3][select])) { // only matters if volca pitch is on
        lastNote = pitchy;
      }
    }
  }  // end mute check
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {  // midi in
  handleNoteOn(channel, pitch, 0);
  ss = 0;
}

void handlepb(byte channel, int val) {
  MIDI.sendPitchBend(val, settings[0][channel - 1]);
  //  pitchBendChange(channel - 1, val);
}

void handlecc(byte channel, byte control, byte value) {    // din cc out
  MIDI.sendControlChange(control, value, settings[0][channel - 1]);
  //  controlChange(channel - 1, control, value);
}

void panic() {
  for (byte t = 0; t < 127; t++) {
    MIDI.sendNoteOff(t, 0, settings[0][select]); //din midi starts at 1
  }
}

/*
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

  void usbmidi() {
  byte t;
  midiEventPacket_t rx = MidiUSB.read();
  switch (rx.header) {
    case 0:
      break; //No pending events
    case 0x9:
      handleNoteOn((rx.byte1 & 0xF) + 1, rx.byte2, rx.byte3);
      break;
    case 0x8:
      handleNoteOff((rx.byte1 & 0xF) + 1, rx.byte2, rx.byte3);
      //(rx.byte1 & 0xF) = chan
      //rx.byte2        //pitch
      //rx.byte3       //velocity
      break;
  }
  }
*/

void setup() {

  delay(2000);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.clearDisplay();
  display.setTextColor(WHITE, BLACK);
  delay(1000);

  pinMode(push, INPUT);
  pinMode(b1, INPUT_PULLUP);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handlecc);
  MIDI.setHandlePitchBend(handlepb);
  // save();  //  UNCOMMENT FIRST UPLOAD, COMMENT AGAIN AND UPLOAD AGAIN
  load();
  disp();

}

void loop() {
  MIDI.read(); //check din midi
  //  usbmidi();  //check usb midi
  wheel();

  if (deb) {
    deb--;
  } else {
    scan();
  }

  if (ss > ssdelay) {
    scrnsvr();
    ss = 0;
  }
  ss++;
}

long mapf(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void disp() {
  display.clearDisplay();        // clear the internal memory

  display.setTextColor(WHITE);
  for (byte h = 0; h < 8; h++) {
    display.setCursor(1 + (h * 16), 1);
    display.print(h + 1);
    display.setCursor(1 + (h * 16), 26);
    display.print(h + 9);
    display.setCursor(1 + (h * 16), 9);
    if (settings[0][h]) {
      display.print(settings[0][h]);
    } else {
      display.print("DR");
    }
    display.setCursor(1 + (h * 16), 34);
    if (settings[0][h + 8]) {
      display.print(settings[0][h + 8]);
    } else {
      display.print("DR");
    }

    if (!settings[1][h]) {
      display.setCursor(2 + (h * 16), 15);
      display.print("x");
    }
    if (!settings[1][8 + h]) {
      display.setCursor(2 + (h * 16), 40);
      display.print("x");
    }

    if (settings[3][h]) {
      display.setCursor(8 + (h * 16), 15);
      display.print("v");
    }
    if (settings[3][8 + h]) {
      display.setCursor(8 + (h * 16), 40);
      display.print("v");
    }

    if (settings[2][h] != 4) {
      if (settings[2][h] < 4) {
        display.drawPixel(13 + (h * 16), 8 + (2 * (3 - settings[2][h])), WHITE);
        display.drawPixel(14 + (h * 16), 9 + (2 * (3 - settings[2][h])), WHITE);
        display.drawPixel(15 + (h * 16), 8 + (2 * (3 - settings[2][h])), WHITE);
      }
      if (settings[2][h] > 4) {
        display.drawPixel(13 + (h * 16), 8 - (2 * (settings[2][h] - 5)), WHITE);
        display.drawPixel(14 + (h * 16), 7 - (2 * (settings[2][h] - 5)), WHITE);
        display.drawPixel(15 + (h * 16), 8 - (2 * (settings[2][h] - 5)), WHITE);
      }
    }
    if (settings[2][h + 8] != 4) {
      if (settings[2][h + 8] < 4) {
        display.drawPixel(13 + (h * 16), 33 + (2 * (3 - settings[2][h + 8])), WHITE);
        display.drawPixel(14 + (h * 16), 34 + (2 * (3 - settings[2][h + 8])), WHITE);
        display.drawPixel(15 + (h * 16), 33 + (2 * (3 - settings[2][h + 8])), WHITE);
      }
      if (settings[2][h + 8] > 4) {
        display.drawPixel(13 + (h * 16), 33 - (2 * (settings[2][h + 8] - 5)), WHITE);
        display.drawPixel(14 + (h * 16), 32 - (2 * (settings[2][h + 8] - 5)), WHITE);
        display.drawPixel(15 + (h * 16), 33 - (2 * (settings[2][h + 8] - 5)), WHITE);
      }
    }
  }// end loop of inputs

  // yellow section for drum filter - yamaha qy to volca sample with pajen firmware
  for (byte i = 0; i < 6; i++) {
    display.setCursor(33 + (i * 16), 48);
    display.print(i + 1);
    display.setCursor(33 + (i * 16), 57);
    display.print(drum[0][i]);
    if (!drum[1][i]) {
      display.setCursor(39 + (i * 16), 48);
      display.print("x");
    }
  }

  display.setCursor(6, 52);
  if (saved) {
    param = 0;
    display.print("S");
  } else if (loaded) {
    param = 0;
    display.print("L");
  }

  switch (select) {
    case 0:
      display.fillRect(0, 0, 16, 17, SSD1306_INVERSE);
      break;
    case 1:
      display.fillRect(16, 0, 16, 17, SSD1306_INVERSE);
      break;
    case 2:
      display.fillRect(32, 0, 16, 17, SSD1306_INVERSE);
      break;
    case 3:
      display.fillRect(48, 0, 16, 17, SSD1306_INVERSE);
      break;
    case 4:
      display.fillRect(64, 0, 16, 17, SSD1306_INVERSE);
      break;
    case 5:
      display.fillRect(80, 0, 16, 17, SSD1306_INVERSE);
      break;
    case 6:
      display.fillRect(96, 0, 16, 17, SSD1306_INVERSE);
      break;
    case 7:
      display.fillRect(112, 0, 16, 17, SSD1306_INVERSE);
      break;

    case 8:
      display.fillRect(0, 25, 16, 17, SSD1306_INVERSE);
      break;
    case 9:
      display.fillRect(16, 25, 16, 17, SSD1306_INVERSE);
      break;
    case 10:
      display.fillRect(32, 25, 16, 17, SSD1306_INVERSE);
      break;
    case 11:
      display.fillRect(48, 25, 16, 17, SSD1306_INVERSE);
      break;
    case 12:
      display.fillRect(64, 25, 16, 17, SSD1306_INVERSE);
      break;
    case 13:
      display.fillRect(80, 25, 16, 17, SSD1306_INVERSE);
      break;
    case 14:
      display.fillRect(96, 25, 16, 17, SSD1306_INVERSE);
      break;
    case 15:
      display.fillRect(112, 25, 16, 17, SSD1306_INVERSE);
      break;

    case 16:
      display.fillRect(32, 48, 16, 17, SSD1306_INVERSE);
      break;
    case 17:
      display.fillRect(48, 48, 16, 17, SSD1306_INVERSE);
      break;
    case 18:
      display.fillRect(64, 48, 16, 17, SSD1306_INVERSE);
      break;
    case 19:
      display.fillRect(80, 48, 16, 17, SSD1306_INVERSE);
      break;
    case 20:
      display.fillRect(96, 48, 16, 17, SSD1306_INVERSE);
      break;
    case 21:
      display.fillRect(112, 48, 16, 17, SSD1306_INVERSE);
      break;
  }

  // lower part of display
  display.setCursor(0, 48);
  switch (param) {
    case 0:
      display.fillRect(0, 48, 16, 17, SSD1306_INVERSE);
      break;
    case 1:
      display.print("out");
      display.setCursor(0, 57);
      if (select < 16) display.print("bnd");
      display.fillRect(0, 56, 18, 11, SSD1306_INVERSE);
      break;
    case 2:
      if (select < 16) {
        display.print("o ");
        display.print(settings[2][select] - 4);
        display.setCursor(0, 57);
        display.print("bdn");
      } else {
        display.print("cc");
        display.setCursor(0, 57);
        display.print(drum[2][select - 16]);
      }
      break;
    case 3:
      display.print("cc");
      display.setCursor(0, 57);
      display.print(settings[4][select]);
      break;
    case 4:
      display.print("vb");
      display.print(vWidth - 24);
      display.setCursor(0, 57);
      display.print("spd");
      break;
    case 5:
      display.print("l/s");
      display.setCursor(0, 57);
      if (settings[3][select]) {
        display.print("b:sp");
      } else {
        display.print("b:n");
      }
      break;
  }

  display.display();        // transfer internal memory to the display
  ss = 0;
}

void clearmessage() {
  param = 0;
  saved = 0;
  loaded = 0;
}

void touch() {

}

void scan() {
  bool unpush = !digitalRead(push);
  int live = analogRead(b1);
  int touch1;

  if (live < 600) {
    switch (param) {
      case 0:
        if (select < 16) {
          if (settings[1][select]) {
            settings[1][select] = 0;
            panic();
            handlepb(settings[0][select], 0);
          } else {
            settings[1][select] = 1;
          }
        } else {
          if (drum[1][select - 16]) {
            drum[1][select - 16] = 0;
          } else {
            drum[1][select - 16] = 1;
          }
        }
        deb = deblimit;
        disp();
        break;
      case 1:
      case 2:
      case 3:
      case 4:
        byte vs;
        touch1 = (1023 - live) - 466 - damp;
        if (touch1 < 0) {
          touch1 = 0;
        }
        if (touch1 != tcheck1) {
          ss = 0;
          if (touch1 > 511) {
            touch1 = 511;
          }
          if (select > 15) {
            MIDI.sendControlChange(drum[2][select - 16], mapf(touch1, 0, 511, 0, 127), select - 15 + ((select == 20) * 4) + ((select == 21) * 4));
          } else {
            if (settings[0][select]) {
              if (param == 1 || param == 2) {
                if (settings[3][select]) {  // if pitch bend is volca sample speed control
                  if (lastNote < 36) {
                    vs = 0;
                  } else {
                    vs = vSpeedValue[lastNote - 36];
                  }
                  MIDI.sendControlChange(43, mapf(touch1, 0, 511, vs, !(param - 1) * 127), settings[0][select]);
                } else {
                  if (param - 1) {
                    handlepb(select + 1, mapf(touch1, 0, 511, 0, -8191));
                  } else {
                    handlepb(select + 1, mapf(touch1, 0, 511, 0, 8191));
                  }
                }
              } // if pitch bend up or down param
              if (param == 3) {  // CC
                MIDI.sendControlChange(settings[4][select], mapf(touch1, 0, 511, 0, 127), settings[0][select]);
              }
              if (param == 4) {  // Vibrato

                if (!touch1) {
                  vState = 0;
                  if (settings[3][select]) {  // if pitch bend is volca sample speed control
                    if (lastNote < 36) {
                      vs = 0;
                    } else {
                      vs = vSpeedValue[lastNote - 36];
                    }
                    MIDI.sendControlChange(43, vs, settings[0][select]);
                  } else {
                    handlepb(select + 1, 0);
                  }
                } else {
                  if (!vState) { // is it new?
                    vState = 1;
                    vPos = 0;
                    if (vWidth < 24) {
                      vWidthFixed = mapf(vWidth, 24, 0, 700, 8191);
                      vDown = 1;
                    } else {
                      vWidthFixed = mapf(vWidth, 24, 48, 700, 8191);
                      vDown = 0;
                    }
                  }  // end new vibe check

                  if (vDelay) {
                    vDelay--;
                  } else {
                    if (vWidthFixed > 3000) {
                      vDelay = (vWidthFixed - 3000) / 20;
                    }
                    if (vDown) {
                      vPos -= touch1 / 6;
                      if (vPos < 0 - vWidthFixed) {
                        vDown = 0;
                        vPos = 0 - vWidthFixed;
                      }
                    } else {
                      vPos += touch1 / 6;
                      if (vPos > vWidthFixed) {
                        vDown = 1;
                        vPos = vWidthFixed;
                      }
                    }

                    if (settings[3][select]) {  // if pitch bend is volca sample speed control
                      if (lastNote < 36) {
                        vs = 0;
                      } else {
                        vs = vSpeedValue[lastNote - 36];
                      }
                      if (vPos > 0) {
                        MIDI.sendControlChange(43, mapf(vPos, 0, 8191, vs, 127), settings[0][select]);
                      } else if (vPos < 0) {
                        MIDI.sendControlChange(43, mapf(vPos, 0, -8191, vs, 0), settings[0][select]);
                      }
                    } else {
                      handlepb(select + 1, vPos);
                    }
                  }

                }  // end non-zero touch pressure
              }  // end vibrato
            } else {  // it's drum filter mode, let's speed cc all the drum channels
              byte dtt;
              for (byte dt = 0; dt < 6; dt ++) {
                dtt = dt;
                if (dt > 3) dtt = dt + 4;
                if (param - 1) {
                  MIDI.sendControlChange(43, mapf(touch1, 0, 511, 64, 0), dtt + 1);
                } else {
                  MIDI.sendControlChange(43, mapf(touch1, 0, 511, 64, 127), dtt + 1);
                }
              }
            }
          }
          tcheck1 = touch1;
        }
        break;
      case 5:
        if (settings[3][select]) {
          settings[3][select] = 0;
        } else {
          settings[3][select] = 1;
        }
        deb = deblimit;
        disp();
        break;
    }
  }

  if (cw) {
    cw = false;
    if ((saved) || (loaded)) {
      clearmessage();
    } else {
      if (unpush) {
        param++;
        if ((select > 15) && (param > 2))  param = 0;
        if (param > 5) {
          param = 0;
          handlepb(settings[0][select], 0);
        }
      } else {
        switch (param) {
          case 0:
            select++;
            if ((select > 15) && (param > 2)) param = 2;
            if (select > 21) select = 0;
            break;
          case 1:
            if (select < 16) {
              panic();
              handlepb(settings[0][select], 0);
              settings[0][select]++;
              if (settings[0][select] > 16) settings[0][select] = 0;
            } else {
              drum[0][select - 16]++;
              if (drum[0][select - 16] > 99) drum[0][select - 16] = 0;
            }
            break;
          case 2:
            if (select < 16) {
              panic();
              settings[2][select]++;
              if (settings[2][select] > 8) settings[2][select] = 8;
            } else {
              drum[2][select - 16]++;
              if (drum[2][select - 16] > 127) drum[2][select - 16] = 0;
            }
            break;
          case 3:
            settings[4][select]++;
            if (settings[4][select] > 127) settings[4][select] = 0;
            break;
          case 4:
            vWidth++;
            if (vWidth > 48) vWidth = 0;
            vState = 0;
            break;
          case 5:
            save();
            break;
        }
      }
    }
    disp();
  }  // end cw

  if (ccw) {
    ccw = false;
    if ((saved) || (loaded)) {
      clearmessage();
    } else {
      if (unpush) {
        param--;
        if ((select > 15) && (param > 2)) param = 2;
        if (param > 5) {
          param = 5;
          handlepb(settings[0][select], 0);
        }
      } else {
        switch (param) {
          case 0:
            select--;
            if (select > 21) {
              select = 21;
              if (param > 2) param = 0;
            }
            break;
          case 1:
            if (select < 16) {
              panic();
              handlepb(settings[0][select], 0);
              settings[0][select]--;
              if (settings[0][select] > 16) settings[0][select] = 15;
            } else {
              drum[0][select - 16]--;
              if (drum[0][select - 16] > 99) drum[0][select - 16] = 99;
            } break;
          case 2:
            if (select < 16) {
              panic();
              settings[2][select]--;
              if (settings[2][select] > 8) settings[2][select] = 0;
            } else {
              drum[2][select - 16]--;
              if (drum[2][select - 16] > 127) drum[2][select - 16] = 127;
            }
            break;
          case 3:
            settings[4][select]--;
            if (settings[4][select] > 127) settings[4][select] = 127;
            break;
          case 4:
            vWidth--;
            if (vWidth > 48) vWidth = 48;
            vState = 0;
            break;
          case 5:
            load();
            break;
        }
      }
    }
    disp();
  }  // end ccw

}  // end scan

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
  for (byte m = 0; m < 16; m++) {
    select = m;
    panic();
    EEPROM.get(2 * m, settings[0][m]);
    EEPROM.get(32 + 2 * m, settings[1][m]);
    EEPROM.get(64 + 2 * m, settings[2][m]);
    EEPROM.get(96 + 2 * m, settings[3][m]);
    EEPROM.get(128 + 2 * m, settings[4][m]);
  }
  for (byte n = 0; n < 6; n++) {
    EEPROM.get(160 + 2 * n, drum[0][n]);
    EEPROM.get(172 + 2 * n, drum[1][n]);
    EEPROM.get(184 + 2 * n, drum[2][n]);
  }
  EEPROM.get(196, vWidth);
  loaded = 1;
}

void save() {
  for (byte m = 0; m < 16; m++) {
    EEPROM.put(2 * m, settings[0][m]);
    EEPROM.put(32 + 2 * m, settings[1][m]);
    EEPROM.put(64 + 2 * m, settings[2][m]);
    EEPROM.put(96 + 2 * m, settings[3][m]);
    EEPROM.put(128 + 2 * m, settings[4][m]);
  }
  for (byte n = 0; n < 6; n++) {
    EEPROM.put(160 + 2 * n, drum[0][n]);
    EEPROM.put(172 + 2 * n, drum[1][n]);
    EEPROM.put(184 + 2 * n, drum[2][n]);
  }
  EEPROM.put(196, vWidth);
  saved = 1;
}

void scrnsvr() {
  long tim;
  int sy;
  int sx;
  int siz;
  ss = 1;
  while (!cw && !ccw && ss != 0 && analogRead(b1) > 400) {
    wheel();
    //scan();
    //   usbmidi();
    MIDI.read();
    if (!tim) {
      display.clearDisplay();        // clear the internal memory
      sy = random(33) + 30;
      sx = 0;
      siz = random(4) + 6;
      while (sx < 127) {
        sx++;
        sy = sy + random(5) - 2;
        if (sy < siz + 4) {
          sy = siz + 4;
        }
        if (sy > 63) {
          sy = 63;
        }
        display.drawPixel(sx, sy, SSD1306_WHITE);
      }
      sx = 0;
      display.display();        // transfer internal memory to the display
    }
    tim++;
    // delay(1);
    if (tim > 100000) tim = 0;
  }
  disp();
}
