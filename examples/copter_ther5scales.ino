// Woz Supposedly of Greasy Conversation - 4-2-21
// copter controller

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include <MIDIUSB.h>
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R2);

#define ssdelay 5000000   // screen saver delay

int channel = 2;
int r = 1;
int xa;
int xb;
int ya;
int yb;
#define distcontrol A3
bool distoff;
int distchange = 0;
#define distfilter 10
int mmap[3] =
{0, 0, 0};
int lmap[3] =
{64, 64, 64};
int swi;
bool off;
long ss; // screen saver time

byte vel = 64;
byte oct = 3;
byte noteOffset = 11;
byte live;
bool pass;
bool keys[12];
char note[] = "C C#D D#E F F#G G#A A#B ";
byte major[7] = {0, 2, 4, 5, 7, 9, 11};
byte minor[7] = {0, 2, 3, 5, 7, 8, 10};
byte blues[7] = {0, 3, 5, 6, 7, 10, 0};
byte Mblues[7] = {0, 2, 3, 4, 7, 9, 0};
byte mode[2];
int oldLive;

void panicer() {
  for (byte t = 12; t < 96; t++) {
    noteOff(channel, t, 0);
  }
}

void clean() {
  pass = false;
  for (int sc = 0; sc < 7; sc++) {
    if ((mode[1] == 0) || (mode[1] == 2)) {
      if ((live % 12) == ((major[sc] + mode[0]) % 12)) {
        pass = true;
      }
    }
    if ((mode[1] == 1) || (mode[1] == 2) || (mode[1] > 5)) {
      if ((live % 12) == (( ((mode[1] == 7) && sc == 5 ) + ((mode[1] > 5) && sc == 6 ) + minor[sc] + mode[0]) % 12)) {
        pass = true;
      }
    }
    if ((mode[1] == 3) || (mode[1] == 5)) {
      if ((live % 12) == ((blues[sc] + mode[0]) % 12)) {
        pass = true;
      }
    }
    if ((mode[1] == 4) || (mode[1] == 5)) {
      if ((live % 12) == ((Mblues[sc] + mode[0]) % 12)) {
        pass = true;
      }
    }
  }
  if (!pass) live++;
}

#define NUM_AI 3
#define ANALOGUE_PIN_ORDER A2, A1, A0
#define MIDI_CC MIDI_CC_GENERAL1

// A knob or slider movement must initially exceed this value to be recognised as an input. Note that it is
// for a 7-bit (0-127) MIDI value.

#define FILTER_AMOUNT 24

// Timeout is in microseconds
#define ANALOGUE_INPUT_CHANGE_TIMEOUT 200000

// Array containing a mapping of analogue pins to channel index. This array size must match NUM_AI above.
byte analogueInputMapping[3] = {ANALOGUE_PIN_ORDER};

// Contains the current value of the analogue inputs.
byte analogueInputs[3];

// Variable to hold temporary analogue values, used for analogue filtering logic.
byte tempAnalogueInput;

// Preallocate the for loop index so we don't keep reallocating it for every program iteration.
byte i = 0;
byte digitalOffset = 0;
// Variable to hold difference between current and new analogue input values.
byte analogueDiff = 0;
// This is used as a flag to indicate that an analogue input is changing.
boolean analogueInputChanging[3];
// Time the analogue input was last moved
unsigned long analogueInputTimer[3];

void controlChange(byte thechannel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | thechannel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

void pitchBendChange(byte thechannel, int value) {
  byte lowValue = value & 0x7F;
  byte highValue = value >> 7;
  midiEventPacket_t event = {0x0E, 0xE0 | thechannel, lowValue, highValue};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void setup() {
  Serial1.begin(9600);
  delay(2000);
  u8g2.begin();

  pinMode(10, INPUT_PULLUP);//buttons bottom
  pinMode(16, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);//trigger buttons
  pinMode(5, INPUT_PULLUP);

  pinMode(7, INPUT_PULLUP);//on switch
  pinMode(8, INPUT_PULLUP);//two way switch
  pinMode(9, INPUT_PULLUP);

  mode[0] = 4;  // preset to Emss
  mode[1] = 1;
}

void oled() {
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_logisoso16_tr);
  if (digitalRead(5)) {
    u8g2.setCursor(66, 13);
    u8g2.print("oc");
    u8g2.setCursor(66, 32);
    u8g2.print(oct);
  } else {
    u8g2.setCursor(66, 13);
    u8g2.print("no");
    u8g2.setCursor(66, 32);
    u8g2.print(noteOffset-11);
  }

  int handle;
  for (int lever = 0; lever < 3; lever++) {              //levers
    u8g2.drawLine(3 + (lever * 8), 3, 3 + (lever * 8), 29 );
    if (lever == swi) {
      handle = 3;
    } else {
      handle = 2;
    }
    u8g2.drawDisc(3 + (lever * 8), 31 - (lmap[lever] / 4), handle, U8G2_DRAW_ALL);
  }

  u8g2.drawDisc(96 + ((mmap[1] + 2) / 8), 32 - (mmap[2] / 8), 3, U8G2_DRAW_ALL); //joystick indicator
  u8g2.drawCircle(113, 15, 5, U8G2_DRAW_ALL);

  if (!distoff) {                                     //distance sensor indicator
    u8g2.drawCircle(60, 5, r, U8G2_DRAW_ALL);
  } else {
    u8g2.drawLine(56, 1, 64, 9);
    u8g2.drawLine(64, 1, 56, 9);
  }

  u8g2.sendBuffer();         // transfer internal memory to the display
}

void buttons() {

  if (!digitalRead(10)) {
    panicer();
    if (digitalRead(4)) {
      mode[0]--;
      if (mode[0] > 11) mode[0] = 11;
    } else {
      mode[1]--;
      if (mode[1] > 7) mode[1] = 7;
    }
    disp();
    delay(300);
  }
  if (!digitalRead(16)) {
    panicer();
    if (digitalRead(4)) {
      mode[0]++;
      if (mode[0] > 11) mode[0] = 0;
    } else {
      mode[1]++;
      if (mode[1] > 7) mode[1] = 0;
    }
    disp();
    delay(300);
  }
  if (!digitalRead(14)) {
    panicer();
    if (digitalRead(5)) {
      oct--;
      if (oct > 5) oct = 5;
    } else {
      noteOffset--;
      if (noteOffset > 22) {
        noteOffset = 11;
        oct--;
      }
    }
    oled();
    delay(300);
  }
  if (!digitalRead(15)) {
    panicer();
    if (digitalRead(5)) {
      oct++;
      if (oct > 5) oct = 0;
    } else {
      noteOffset++;
      if (noteOffset > 22){
        noteOffset = 11;
        oct++;
      }
    }
    oled();
    delay(300);
  }

}
void loop() {

  if (!digitalRead(8)) {
    if (swi != 0) {
      swi = 0;
      oled();
    }
  }
  if (!digitalRead(9)) {
    if (swi != 2) {
      swi = 2;
      oled();
    }
  }
  if (digitalRead(8) && digitalRead(9)) {
    if (swi != 1) {
      swi = 1;
      oled();
    }
  }

  if ((!digitalRead(5))||(!digitalRead(4))) {
    if (distoff) {
      distoff = false;
      noteOff(channel, oldLive, 0);      //     ///     ////    /////
      MidiUSB.flush();
      oled();
    }
    int distcheck = analogRead(A3);
    if (distcheck < 150) {
      distcheck = 0;
      if (!off) {
        off = true;
        noteOff(channel, oldLive, 0);      //     ///     ////    /////
        MidiUSB.flush();
      }
    }
    if ((distcheck < distchange - distfilter) || (distcheck > distchange + distfilter)) {
      off = false;
      if (distcheck > 1000) {
        distcheck = 0;
      }
      int dist = ((distcheck - 135) / 4.5);
      if (dist > 127) {
        dist = 127;
      }
      if (dist < 0) {
        dist = 0;
      }
      r = dist / 2;
      live = (dist / 7) + 24 + (oct * 12) + noteOffset;
      clean();
      clean();
      noteOff(channel, oldLive, 0);      //     ///     ////    /////
      MidiUSB.flush();
      noteOn(channel, live, vel);
      MidiUSB.flush();
      distchange = distcheck;
      oldLive = live;
      oled();
    }
  } else {
    if (!distoff) {
      distoff = true;
      r = 0;
      noteOff(channel, oldLive, 0);      //     ///     ////    /////
      MidiUSB.flush();
      oled();
    }
  }

  buttons();

  for (i = 0; i < 3; i++)
  {
    // Read the analogue input pin, dividing it by 8 so the 10-bit ADC value (0-1023) is converted to a 7-bit MIDI value (0-127).
    tempAnalogueInput = analogRead(analogueInputMapping[i]) / 4;

    // Take the absolute value of the difference between the curent and new values
    analogueDiff = abs(tempAnalogueInput - analogueInputs[i]);
    // Only continue if the threshold was exceeded, or the input was already changing
    if ((analogueDiff > 0 && analogueInputChanging[i] == true) || analogueDiff >= FILTER_AMOUNT)
    {
      // Only restart the timer if we're sure the input isn't 'between' a value
      // ie. It's moved more than FILTER_AMOUNT
      if (analogueInputChanging[i] == false || analogueDiff >= FILTER_AMOUNT)
      {
        // Reset the last time the input was moved
        analogueInputTimer[i] = micros();

        // The analogue input is moving
        analogueInputChanging[i] = true;
      }
      else if (micros() - analogueInputTimer[i] > ANALOGUE_INPUT_CHANGE_TIMEOUT)
      {
        analogueInputChanging[i] = false;
      }

      // Only send data if we know the analogue input is moving
      if (analogueInputChanging[i] == true)
      {
        // Record the new analogue value
        analogueInputs[i] = tempAnalogueInput;

        mmap[i] = tempAnalogueInput;

        u8g2.setCursor(27, 16);                            //text location

        if (i == 0) {
          lmap[swi] = tempAnalogueInput / 2;
          if (swi == 0) {
            vel = tempAnalogueInput / 2;
            u8g2.print("Vel");
            u8g2.setCursor(27, 32);
            u8g2.print(vel);
          }
          if (swi == 1) {
            int vol = tempAnalogueInput / 2;
            controlChange(channel, 7, vol);
            u8g2.print("Vol");
            u8g2.setCursor(27, 32);
            u8g2.print(vol);
          }
          if (swi == 2) {
            controlChange(channel, 8, tempAnalogueInput / 2);
            u8g2.print("CC8");
            u8g2.setCursor(27, 32);
            u8g2.print(tempAnalogueInput / 2);
          }
          u8g2.sendBuffer();         // transfer internal memory to the display
        }

        if (i == 1) {
          if (tempAnalogueInput < 139) {          //left
            xa = round((136 - tempAnalogueInput) * 1.15) - 2;
            if (xa > 127) {
              xa = 127;
            }
            if (xa < 0) {
              xa = 0;
            }
            pitchBendChange(channel, map(xa, 0, 127, 8191, 0));
          }
          if (tempAnalogueInput > 136) {         //right
            xb = round((tempAnalogueInput - 136) * 1.11) - 2;
            if (xb > 127) {
              xb = 127;
            }
            if (xb < 0) {
              xb = 0;
            }
            pitchBendChange(channel, map(xb, 0, 127, 8191, 16383));
          }
        }

        if (i == 2) {
          if (tempAnalogueInput < 139) {        //bottom
            ya = round((138 - tempAnalogueInput) * .95) - 2;
            if (ya > 127) {
              ya = 127;
            }
            if (ya < 0) {
              ya = 0;
            }
            controlChange(channel, 1, ya);
          }
          if (tempAnalogueInput > 136) {        //top
            yb = round((tempAnalogueInput - 136) * 1.11) - 2;
            if (yb > 127) {
              yb = 127;
            }
            if (yb < 0) {
              yb = 0;
            }
            controlChange(channel, 2, yb);
          }
        }

        oled();

      }
    }
  }

  if (ss > ssdelay) {
    scrnsvr();
    ss = 0;
  }
  ss++;

}

void scrnsvr() {
  long tim;
  int sy;
  int sx;
  int siz;
  int moon;
  ss = 1;
  while (!digitalRead(5) && ss != 0) {
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
          small(sx);
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
  oled();
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

void keyupdate() {
  for (int j = 0; j < 12; j++) {
    keys[j] = 0;
  }
  for (int sc = 0; sc < 7; sc++) {
    if ((mode[1] == 0) || (mode[1] == 2)) {
      keys[((major[sc] + mode[0]) % 12)] = true;
    }
    if ((mode[1] == 1) || (mode[1] == 2) || (mode[1] > 5)) {
      keys[((((mode[1] == 7) && sc == 5 ) + ((mode[1] > 5) && sc == 6 ) + minor[sc] + mode[0]) % 12)] = true;
    }
    if ((mode[1] == 3) || (mode[1] == 5)) {
      keys[((blues[sc] + mode[0]) % 12)] = true;
    }
    if ((mode[1] == 4) || (mode[1] == 5)) {
      keys[((Mblues[sc] + mode[0]) % 12)] = true;
    }
  }
}

void disp() {
  u8g2.clearBuffer();          // clear the internal memory

  keyupdate();
  drawkb();

  small(0);
  ss = 0;
  u8g2.sendBuffer();           // transfer internal memory to the display
}

void small(int lang) {
  int n = ((mode[0] % 12) * 2);
  u8g2.setCursor(lang, 15);            //      //
  u8g2.print(note[n]);
  u8g2.print(note[n + 1]);
  u8g2.setCursor(lang, 33);
  if ((mode[1] > 2) && (mode[1] < 6)) u8g2.print("B");
  if (mode[1] == 0 || mode[1] == 2 || ((mode[1] > 3) && (mode[1] < 6))) u8g2.print("M");
  if (mode[1] == 6) u8g2.print("H");
  if (mode[1] == 7) u8g2.print("m");
  if (mode[1] == 1 || mode[1] == 2 || mode[1] > 4) u8g2.print("m");
  u8g2.print(" ");
}
