// UNCOMMENT LINE 90 the first time you upload this code, then put it back
// woz.lol piezo drum pad code for Pro Micro / Leonardo
// 8 piezo elements, 1 analog knob, and one switch, all assignable to 4 banks
// push the rotary encoder to choose parameters

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
Rotary r = Rotary(15, 14);
#define MIDI_SERIAL_PORT Serial1
#define MIDI_CC MIDI_CC_GENERAL1
#define ssdelay 24000   // screen saver delay
#define pot A10
#define swtch 16
#define push 5
byte thresh = 160;   // min pad sensitivity, 70-160-200
#define kfilter 10    //knob movement filter
#define ANALOGUE_PIN_ORDER A6, A7, A8, A9, A0, A1, A2, A3
byte pin[8] = {ANALOGUE_PIN_ORDER};
int hitavg = 0;
byte chan = 15;     // starting midi channel -1
bool cw;
bool ccw;
bool unpush;  //true when knob is not pushed
byte bank;
byte param = 3;
byte pad[4][10] = {  // bank x pad notes 1-8, then the cc's for the knob then the switch
  {29, 31, 35, 36, 37, 38, 40, 44, 1, 77},
  {33, 34, 35, 38, 40, 42, 44, 46, 2, 78},
  {64, 65, 66, 70, 73, 75, 76, 77, 11, 79},
  {36, 38, 40, 41, 43, 45, 47, 48, 7, 80},
};
String note = "C C#D D#E F F#G G#A A#B ";
int ss; // screen saver time
int k;
bool sw;

MIDI_CREATE_DEFAULT_INSTANCE();

void controlChange(byte thechannel, byte control, byte value) {
  MIDI.sendControlChange(control, value, thechannel + 1);
  midiEventPacket_t event = {0x0B, 0xB0 | thechannel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

void noteOn(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOn(pitch, velocity, channel + 1);
  midiEventPacket_t on = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(on);
  MidiUSB.flush();
  delay(velocity);
  MIDI.sendNoteOn(pitch, 0, channel + 1);
  midiEventPacket_t off = {0x09, 0x90 | channel, pitch, 0};
  MidiUSB.sendMIDI(off);
  MidiUSB.flush();
  delay(4);
  ss = 0;
}

void setup() {
  delay(2000);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.clearDisplay();
  display.setTextColor(WHITE, BLACK);
  Wire.begin(); //Join the bus as master.
  Wire.setClock(3400000);
  for (int px = 0; px < 8; px++) {
    pinMode(pin[px], INPUT);
  }

  pinMode(pot, INPUT);
  pinMode(swtch, INPUT_PULLUP);
  pinMode(push, INPUT);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  
  for (int tchan = 0; tchan < 16; tchan++) {
  for (int unote = 0; unote < 127; unote++) {
    MIDI.send(128, unote, 0, tchan + 1); //turn note off
    delay(2);
  }
  }
  

  // !!! UNCOMMENT THE LINE BELOW ON FIRST RUN OF THIS CODE, then comment it again and upload this a second time
  // ewrite();

  EEPROM.get(0, chan);
  EEPROM.get(2, thresh);

  for (byte sh = 0; sh < 4; sh++) {
    for (byte sw = 0; sw < 10; sw++) {
      EEPROM.get(4 + 2 * (sw + (10 * sh)), pad[sh][sw]);
    }
  }

  disp();
}

void loop() {
  MIDI.read();
  wheel();
  scan();
  pads();

  if (ss > ssdelay) {
    scrnsvr();
  }
  ss++;

}

void disp() {
  byte p1;
  display.clearDisplay();        // clear the internal memory

  display.setCursor(1, 48);
  display.print("s");
  display.print(pad[bank][9]);
  display.setCursor(7, 56);
  display.print(sw * 127);
  display.setCursor(33, 48);
  display.print("k");
  display.print(pad[bank][8]);
  display.setCursor(39, 56);
  display.print(k / 8);

  display.setCursor(65, 48);
  display.print("chan");
  display.setCursor(71, 56);
  display.print(chan + 1);

  display.setCursor(97, 48);

  if (param == 12) {
    display.print("trim");
    display.setCursor(97, 56);
    display.print(thresh);
  } else if (param == 13) {
    display.setCursor(97, 56);
    display.print("save");
  } else {
    display.print("bank");
    display.setCursor(97 + (bank * 6), 56);
    display.print(bank + 1);
  }

  for (byte pdo = 0; pdo < 8; pdo++) {
    bool pdo4 = pdo > 3;
    display.setCursor(1 + (pdo * 32) - (pdo4 * 128), 5 + (pdo4 * 24));
    if (pad[bank][pdo] < 100) {
      display.print(pad[bank][pdo]);
    } else {
      display.drawLine(1 + (pdo * 32) - (pdo4 * 128), 1 + (pdo4 * 24), 1 + (pdo * 32) - (pdo4 * 128), 3 + (pdo4 * 24), SSD1306_WHITE);
      if (pad[bank][pdo] < 110) display.print("0");
      display.print(pad[bank][pdo] - 100);
    }
    p1 = (pad[bank][pdo] % 12) * 2;
    display.setCursor(13 + (pdo * 32) - (pdo4 * 128), 1 + (pdo4 * 24));
    display.print(note.substring(p1, p1 + 2));
    int oc = (pad[bank][pdo] / 12) - 1;
    if (oc == -1) {
      display.print("-");
    } else {
      display.print(oc);
    }
  }

  for (byte l = 0; l < 4; l++) {
    for (byte d = 0; d < 17; d += 2) {
      display.drawPixel(31 + (l * 32), d, SSD1306_WHITE); // horizontal lines
      display.drawPixel(31 + (l * 32), 23 + d, SSD1306_WHITE);
    }
  }

  switch (param) {
    case 0:
      display.fillRect(0, 48, 25, 18, SSD1306_INVERSE);
      break;
    case 1:
      display.fillRect(32, 48, 25, 18, SSD1306_INVERSE);
      break;
    case 2:
      display.fillRect(64, 48, 25, 18, SSD1306_INVERSE);
      break;
    case 3:
      display.fillRect(96, 48, 25, 18, SSD1306_INVERSE);
      break;
    case 4:
      display.fillRect(0, 0, 31, 18, SSD1306_INVERSE);
      break;
    case 5:
      display.fillRect(32, 0, 31, 18, SSD1306_INVERSE);
      break;
    case 6:
      display.fillRect(64, 0, 31, 18, SSD1306_INVERSE);
      break;
    case 7:
      display.fillRect(96, 0, 31, 18, SSD1306_INVERSE);
      break;
    case 8:
      display.fillRect(0, 23, 31, 18, SSD1306_INVERSE);
      break;
    case 9:
      display.fillRect(32, 23, 31, 18, SSD1306_INVERSE);
      break;
    case 10:
      display.fillRect(64, 23, 31, 18, SSD1306_INVERSE);
      break;
    case 11:
      display.fillRect(96, 23, 31, 18, SSD1306_INVERSE);
      break;
    case 12:
      display.fillRect(122, 48, 6, 8, SSD1306_INVERSE);
      break;
    case 13:
      display.fillRect(122, 56, 6, 8, SSD1306_INVERSE);
      break;
  }

  display.display();        // transfer internal memory to the display
  ss = 0;
}

void scan() {
  unpush = !digitalRead(push);
  int know = analogRead(pot);
  int swnow = digitalRead(swtch);
  if (( know > k + kfilter) || (know < k - kfilter)) {
    k = know;
    controlChange(chan, pad[bank][8], k / 8);
    disp();
  }
  if (sw != swnow) {
    sw = swnow;
    controlChange(chan, pad[bank][9], sw * 127);
    disp();
  }

  if (cw) {
    cw = false;
    if (unpush) {
      param++;
      if (param > 13) param = 0;
    } else {
      if (param == 0) {
        pad[bank][9]++;
        if (pad[bank][9] > 127) pad[bank][9] = 0;
      } //end param 0
      if (param == 1) {
        pad[bank][8]++;
        if (pad[bank][8] > 127) pad[bank][8] = 0;
      } //end param
      if (param == 2) {
        chan++;
        if (chan > 15) chan = 0;
      } //end param
      if (param == 3) {
        bank++;
        if (bank > 3) bank = 0;
      } //end param
      if (param == 4) {
        pad[bank][0]++;
        if (pad[bank][0] > 127) pad[bank][0] = 0;
      } //end param
      if (param == 5) {
        pad[bank][1]++;
        if (pad[bank][1] > 127) pad[bank][1] = 0;
      } //end param
      if (param == 6) {
        pad[bank][2]++;
        if (pad[bank][2] > 127) pad[bank][2] = 0;
      } //end param
      if (param == 7) {
        pad[bank][3]++;
        if (pad[bank][3] > 127) pad[bank][3] = 0;
      } //end param
      if (param == 8) {
        pad[bank][4]++;
        if (pad[bank][4] > 127) pad[bank][4] = 0;
      } //end param
      if (param == 9) {
        pad[bank][5]++;
        if (pad[bank][5] > 127) pad[bank][5] = 0;
      } //end param
      if (param == 10) {
        pad[bank][6]++;
        if (pad[bank][6] > 127) pad[bank][6] = 0;
      } //end param
      if (param == 11) {
        pad[bank][7]++;
        if (pad[bank][7] > 127) pad[bank][7] = 0;
      } //end param
      if (param == 12) {
        thresh++;
        if (thresh > 254) thresh = 70;
      } //end param
      if (param == 13) {
        ewrite();
      } //end param
    } //end not pushed
    disp();
  }  // end cw

  if (ccw) {
    ccw = false;
    if (unpush) {
      param--;
      if (param > 13) param = 13;
    } else {
      if (param == 0) {
        pad[bank][9]--;
        if (pad[bank][9] > 127) pad[bank][9] = 127;
      } //end param 0
      if (param == 1) {
        pad[bank][8]--;
        if (pad[bank][8] > 127) pad[bank][8] = 127;
      } //end param
      if (param == 2) {
        chan--;
        if (chan > 15) chan = 15;
      } //end param
      if (param == 3) {
        bank--;
        if (bank > 3) bank = 3;
      } //end param
      if (param == 4) {
        pad[bank][0]--;
        if (pad[bank][0] > 127) pad[bank][0] = 127;
      } //end param
      if (param == 5) {
        pad[bank][1]--;
        if (pad[bank][1] > 127) pad[bank][1] = 127;
      } //end param
      if (param == 6) {
        pad[bank][2]--;
        if (pad[bank][2] > 127) pad[bank][2] = 127;
      } //end param
      if (param == 7) {
        pad[bank][3]--;
        if (pad[bank][3] > 127) pad[bank][3] = 127;
      } //end param
      if (param == 8) {
        pad[bank][4]--;
        if (pad[bank][4] > 127) pad[bank][4] = 127;
      } //end param
      if (param == 9) {
        pad[bank][5]--;
        if (pad[bank][5] > 127) pad[bank][5] = 127;
      } //end param
      if (param == 10) {
        pad[bank][6]--;
        if (pad[bank][6] > 127) pad[bank][6] = 127;
      } //end param
      if (param == 11) {
        pad[bank][7]--;
        if (pad[bank][7] > 127) pad[bank][7] = 127;
      } //end param
      if (param == 12) {
        thresh--;
        if (thresh < 70) thresh = 254;
      } //end param
    } //end not pushed
    disp();
  }  // end ccw
}

void pads() {

  for (int padscan = 0; padscan < 8; padscan ++) {
    int pd = analogRead(pin[padscan]);
    if (pd > thresh) {
      int vel = pd / 4;
      if (vel > 127) vel = 127;
      noteOn(chan, pad[bank][padscan], vel);
      if (padscan < 4) {
        display.setCursor(12 + (padscan * 32), 9);
      } else {
        display.setCursor(12 + ((padscan - 4) * 32), 32);
      }
      if (vel < 100) display.print(" ");
      display.print(vel);
      display.display();        // transfer internal memory to the display
    }
  }

}

void scrnsvr() {
  int tim;
  int sy;
  int sx;
  int siz;
  int moon;
  ss = 1;
  while (!cw && !ccw && ss != 0) {
    wheel();
    scan();
    pads();
    if (!tim) {
      display.clearDisplay();        // clear the internal memory
      sy = random(34) + 30;
      sx = 0;
      siz = random(4) + 6;
      moon = random(112) + 8;
      while (sx < 127) {
        sx++;
        sy = sy + random(5) - 2;
        if (sy < siz + 4) {
          sy = siz + 4;
        }
        if (sy > 63) {
          sy = 63;
        }
        if (sx == moon)    display.fillCircle(sx, sy - random(8) - 8, siz, SSD1306_WHITE);
        display.drawPixel(sx, sy, SSD1306_WHITE);
      }
      sx = 0;
      for (int a = 0; a < 18; a = a + 3) {
        display.drawPixel(random(128), random(16), SSD1306_WHITE);
      }
      display.display();        // transfer internal memory to the display
    }
    tim++;
    delay(1);
    if (tim > 2000) tim = 0;
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

void ewrite() {

  EEPROM.put(0, chan);
  EEPROM.put(2, thresh);

  for (byte sh = 0; sh < 4; sh++) {
    for (byte sw = 0; sw < 10; sw++) {
      EEPROM.put(4 + 2 * (sw + (10 * sh)), pad[sh][sw]);
    }
  }

  display.setCursor(92, 48);
  display.print(" saved ");
  display.display();
  delay(600);


}
