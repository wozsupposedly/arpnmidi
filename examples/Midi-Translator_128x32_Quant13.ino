// UNCOMMENT LINE 165 the first time you upload this code, then put it back
// woz.lol Midi Cheater Transposer Quantizer Game Changer with smaller 128x32 oled
// this version outputs din and usb midi, with din midi in
#include <U8g2lib.h>
#include <MIDI.h>
#include <SPI.h>
#include <Wire.h>
#include <Rotary.h>
#include <EEPROM.h>
#include <MIDIUSB.h>
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R2);
Rotary r = Rotary(14, 15);
#define MIDI_SERIAL_PORT Serial1
#define MIDI_CC MIDI_CC_GENERAL1
#define nm 41 //guitar dots horz position
#define ssdelay 5000000   // screen saver delay
#define push 16
#define knob A10
#define deblimit 12000
#define kthresh 15
#define foot 4
int oldknob;
bool oldft;
bool cw;
bool ccw;
bool unpush;  //true when knob is not pushed
byte btn;
byte guit;
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

void panic() {
  for (byte t = 36; t < 97; t++) {
    MIDI.sendNoteOff(t, 0, chan + 1); //din midi starts at 1
    midiEventPacket_t off = {0x08, 0x80 | chan, t, 0};
    MidiUSB.sendMIDI(off);
  }
  MidiUSB.flush();
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
  u8g2.begin();
  u8g2.setFlipMode(1);
  u8g2.setDrawColor(1);
  u8g2.setFontMode(0);
  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.clearBuffer();          // clear the internal memory

  Wire.begin(); //Join the bus as master.
  Wire.setClock(3400000);

  pinMode(push, INPUT);
  pinMode(knob, INPUT);
  pinMode(foot, INPUT_PULLUP);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handlecc);
  MIDI.setHandlePitchBend(handlepb);
  // !!! UNCOMMENT THE LINE BELOW ON FIRST RUN OF THIS CODE, then comment it again and upload this a second time
  // save();

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
  //fretts
  u8g2.drawLine(45, 0, 45, 31);  //nut
  for (byte c = 0; c < 12; c++) {
    for (byte f = 0; f < 32; f += 2) {
      u8g2.drawPixel(44 + (7 * c), f);
    }
  }
  //markers

  switch (guit) {
    case 1:
      u8g2.drawLine(59, 8, 65, 8);
      u8g2.drawLine(73, 8, 79, 8);
      u8g2.drawLine(87, 8, 93, 8);
      u8g2.drawLine(101, 8, 107, 8);
      u8g2.drawLine(122, 16, 128, 16);
      break;
    case 2:
      u8g2.drawLine(59, 24, 65, 24);
      u8g2.drawLine(73, 24, 79, 24);
      u8g2.drawLine(87, 24, 93, 24);
      u8g2.drawLine(101, 24, 107, 24);
      u8g2.drawLine(122, 16, 128, 16);
      break;
    case 3:
      u8g2.drawLine(59, 16, 65, 16);
      u8g2.drawLine(73, 16, 79, 16);
      u8g2.drawLine(87, 16, 93, 16);
      u8g2.drawLine(101, 16, 107, 16);
      u8g2.drawLine(122, 8, 128, 8);
      u8g2.drawLine(122, 24, 128, 24);
      break;
  }

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

    if (guit == 1) {  //  E A D G bass and lowest 4 strings of guitar
      if (ae == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 28, keys[ae] * 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawDisc(nm + (7 * a), 28, keys[ae] * 2, U8G2_DRAW_ALL);
      }
      if (aa == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 20, keys[aa] * 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawDisc(nm + (7 * a), 20, keys[aa] * 2, U8G2_DRAW_ALL);
      }
      if (ad == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 12, keys[ad] * 2, U8G2_DRAW_ALL);
      }
      else {
        u8g2.drawDisc(nm + (7 * a), 12, keys[ad] * 2, U8G2_DRAW_ALL);
      }
      if (ag == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 4, keys[ag] * 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawDisc(nm + (7 * a), 4, keys[ag] * 2, U8G2_DRAW_ALL);
      }
    }

    if (guit == 2) {  //   D G B E highest 4 strings of guitar
      if (ad == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 28, keys[ad] * 2, U8G2_DRAW_ALL);
      }
      else {
        u8g2.drawDisc(nm + (7 * a), 28, keys[ad] * 2, U8G2_DRAW_ALL);
      }
      if (ag == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 20, keys[ag] * 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawDisc(nm + (7 * a), 20, keys[ag] * 2, U8G2_DRAW_ALL);
      }
      if (ab == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 12, keys[ab] * 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawDisc(nm + (7 * a), 12, keys[ab] * 2, U8G2_DRAW_ALL);
      }
      if (ae == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 4, keys[ae] * 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawDisc(nm + (7 * a), 4, keys[ae] * 2, U8G2_DRAW_ALL);
      }
    }

    if (guit == 3) {  //  G D E A mandolin / violin
      if (ae == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 4, keys[ae] * 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawDisc(nm + (7 * a), 4, keys[ae] * 2, U8G2_DRAW_ALL);
      }
      if (aa == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 12, keys[aa] * 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawDisc(nm + (7 * a), 12, keys[aa] * 2, U8G2_DRAW_ALL);
      }
      if (ad == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 20, keys[ad] * 2, U8G2_DRAW_ALL);
      }
      else {
        u8g2.drawDisc(nm + (7 * a), 20, keys[ad] * 2, U8G2_DRAW_ALL);
      }
      if (ag == mode[0][btn - 1]) {
        u8g2.drawCircle(nm + (7 * a), 28, keys[ag] * 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawDisc(nm + (7 * a), 28, keys[ag] * 2, U8G2_DRAW_ALL);
      }
    }

  }
}

void drawkb() {
  if (keys[0]) {
    u8g2.drawRBox(36, 16, 7, 16, 3);
  } else {
    u8g2.drawRFrame(36, 16, 7, 16, 3);
  }
  if (keys[2]) {
    u8g2.drawRBox(50, 16, 7, 16, 3);
  } else {
    u8g2.drawRFrame(50, 16, 7, 16, 3);
  }
  if (keys[4]) {
    u8g2.drawRBox(64, 16, 7, 16, 3);
  } else {
    u8g2.drawRFrame(64, 16, 7, 16, 3);
  }
  if (keys[5]) {
    u8g2.drawRBox(78, 16, 7, 16, 3);
  } else {
    u8g2.drawRFrame(78, 16, 7, 16, 3);
  }
  if (keys[7]) {
    u8g2.drawRBox(92, 16, 7, 16, 3);
  } else {
    u8g2.drawRFrame(92, 16, 7, 16, 3);
  }
  if (keys[9]) {
    u8g2.drawRBox(106, 16, 7, 16, 3);
  } else {
    u8g2.drawRFrame(106, 16, 7, 16, 3);
  }
  if (keys[11]) {
    u8g2.drawRBox(120, 16, 7, 16, 3);
  } else {
    u8g2.drawRFrame(120, 16, 7, 16, 3);
  }

  if (keys[1]) {
    u8g2.drawRBox(43, 0, 7, 16, 3);
  } else {
    u8g2.drawRFrame(43, 0, 7, 16, 3);
  }
  if (keys[3]) {
    u8g2.drawRBox(57, 0, 7, 16, 3);
  } else {
    u8g2.drawRFrame(57, 0, 7, 16, 3);
  }
  if (keys[6]) {
    u8g2.drawRBox(85, 0, 7, 16, 3);
  } else {
    u8g2.drawRFrame(85, 0, 7, 16, 3);
  }
  if (keys[8]) {
    u8g2.drawRBox(99, 0, 7, 16, 3);
  } else {
    u8g2.drawRFrame(99, 0, 7, 16, 3);
  }
  if (keys[10]) {
    u8g2.drawRBox(113, 0, 7, 16, 3);
  } else {
    u8g2.drawRFrame(113, 0, 7, 16, 3);
  }

}

void small(byte but, byte pos) {
  int n = ((mode[0][but] % 12) * 2);
  u8g2.setCursor(pos, 15);
  u8g2.print(note[n]);
  u8g2.print(note[n + 1]);
  u8g2.setCursor(pos, 33);
  if ((mode[1][but] > 2) && (mode[1][but] < 6)) u8g2.print("B");
  if (mode[1][but] == 0 || mode[1][but] == 2 || ((mode[1][but] > 3) && (mode[1][but] < 6))) u8g2.print("M");
  if (mode[1][but] == 6) u8g2.print("H");
  if (mode[1][but] == 7) u8g2.print("m");
  if (mode[1][but] == 1 || mode[1][but] == 2 || mode[1][but] > 4) u8g2.print("m");
}

void disp() {
  u8g2.clearBuffer();          // clear the internal memory

  if (!btn) {
    if (saved) {
      u8g2.setCursor(38, 20);
      u8g2.print("SAVED");
      saved = false;
    } else if (loaded) {
      u8g2.setCursor(35, 20);
      u8g2.print("LOADED");
      loaded = false;
    } else {
      u8g2.drawLine(guit * 32, 0, 31 + guit * 32, 0);
      for (byte al = 0; al < 4; al++) {
        small(al, al * 32);
      }
    }
  } else {

    keyupdate();

    if (guit) {
      gtr();
    } else {
      drawkb();
    }
  }

  if (btn) {
    small(btn - 1, 0);
  }

  ss = 0;
  u8g2.sendBuffer();           // transfer internal memory to the display
}

void scan() {
  byte temp;
  bool ft = digitalRead(foot);
  int kn = analogRead(knob);
  unpush = digitalRead(push);
  if ((kn > oldknob + kthresh) || (kn < oldknob - kthresh)) {
    btn = round(kn / 205);
    //guit = round((kn % 205) / 51);
    oldknob = kn;
    panic();
    disp();
  }

  if ((ft) && (!oldft)) {
    btn = ((btn + 1) % 5);
    oldft = true;
    panic();
    disp();
  }
  if ((oldft) && (!ft)) {
    btn = round(kn / 205);
    oldknob = kn;
    oldft = false;
    panic( );
    disp();
  }


  if (cw) {
    cw = false;
    if (!btn) {
      if (unpush) {
        guit++;
        if (guit > 3) guit = 0;
      } else {
        save(); // if param == 2, we load
      }
    } else {
      panic();
      mode[!unpush][btn - 1] ++;
      if (mode[0][btn - 1] > 11 || mode[1][btn - 1] > 7) mode[!unpush][btn - 1] = 0;
    }
    disp();
  }  // end cw

  if (ccw) {
    ccw = false;
    if (!btn) {
      if (unpush) {
        guit--;
        if (guit > 3) guit = 3;
      } else {
        load(); // if param == 2, we load
      }
    } else {
      panic();
      mode[!unpush][btn - 1] --;
      if (mode[0][btn - 1] > 11) mode[!unpush][btn - 1] = 11;
      if (mode[1][btn - 1] > 7) mode[!unpush][btn - 1] = 7;
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
      u8g2.clearBuffer();          // clear the internal memory
      sy = random(16) + 8;
      sx = 0;
      siz = random(4) + 6;
      moon = random(100) + 8;
      while (sx < 127) {
        sx++;
        sy = sy + random(5) - 2;
        if (sy < siz + 4) {
          sy = siz + 4;
        }
        if (sy > 31) {
          sy = 31;
        }
        if (sx == moon) {
          if (btn) {
            small(btn - 1, sx - 8);
          } else {
            u8g2.drawDisc(sx, sy - random(8) - 8, siz, U8G2_DRAW_ALL);
          }
        }
        u8g2.drawPixel(sx, sy);
      }
      sx = 0;
      u8g2.sendBuffer();
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
