
/*
   musicstar keyboard
  woz.lol zx sensor midi filter
  volca sample mode is not needed anymore because of the pajen firmware
  WORKS

*/
#include <EEPROM.h>
#include <MIDIUSB.h>
#include <qdec.h>
#include <ss_oled.h>
#define USE_BACKBUFFER
#define GROVE_SDA_PIN 32
#define GROVE_SCL_PIN 26
#define RESET_PIN -1
#define OLED_ADDR -1
#define FLIP180 1
#define INVERT 0
#define USE_HW_I2C 1
#define MY_OLED OLED_128x64
uint8_t ucBackBuffer[1024];
SSOLED ssoled;
char szTemp[32];

#include <Wire.h>
#include <ZX_Sensor.h>
#include <SPI.h>

#define ROTARY_PIN_A 9 // the first pin connected to the rotary encoder
#define ROTARY_PIN_B 8 // the second pin connected to the rotary encoder
::SimpleHacks::QDecoder qdec(ROTARY_PIN_A, ROTARY_PIN_B, true);

const int ZX_ADDR = 0x10;
ZX_Sensor zx_sensor = ZX_Sensor(ZX_ADDR);
#define MIDI_CC MIDI_CC_GENERAL1
#define s1up 16
#define s1down 14
#define s2up 15     // momentary
#define s2down 5
#define push 7 //knob push
#define d1 18
#define d2 19
#define d3 20
#define d4 21
#define d5 1
#define fl A7  // left force sensor pin
#define fr A6  // right force sensor pin
#define damp 0 // sensitivity adjustment for touch force sensors. lower number is MORE sensitive
#define ssdelay 240000   // screen saver delay

byte chan;     // starting midi channel
byte x;
byte y;
bool s1ul;
bool s1dl;
byte ccZero;
bool sensorLive;
bool cw;
bool ccw;
bool unpush;  //true when knob is not pushed
byte param;
int midiByte = 0 ;
byte mode = 3;
int xxp;
int vv;
int ss; // screen saver time
int tcheck1;
int tcheck2;
int touch1;
int touch2;

char note[] = "C C#D D#E F F#G G#A A#B ";
byte major[7] = {0, 2, 4, 5, 7, 9, 11};
byte minor[7] = {0, 2, 3, 5, 7, 8, 10};
//byte hminor[7] = {0, 2, 3, 5, 7, 8, 11};
//byte mminor[7] = {0, 2, 3, 5, 7, 9, 11};
byte blues[7] = {0, 3, 5, 6, 7, 10, 0};
byte Mblues[7] = {0, 2, 3, 4, 7, 9, 0};
byte mode[2];
byte vel;  // current ther velocity

byte vol1 = 96;
byte cct = 70;
byte ccv = 0;

byte volca;  // if it = 2, that's slow mode/safe mode for the force sensors
bool lefty;
byte trm; // sensitivity trim adjustment

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

void noteUSB(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t notes = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(notes);
}

void noteOn(byte channel, byte pitch, byte velocity) {  // din midi in
  if ((trans) && (chan == channel)) {
    if (trans == 1) pitch = (pitch / 2) + 24;
    live = pitch;
    clean();
    clean();
    noteUSB(chan, live, velocity);
    MidiUSB.flush();
  }
  ss = 0;
}

void noteOff(byte channel, byte pitch, byte velocity) {
  if (chan < 17) {
    noteOn(chan, pitch, 0);
  }
}

void clean() {
  pass = false;
  for (byte sc = 0; sc < 7; sc++) {
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

void setup() {
  char *msgs[] = {(char *)"SSD1306 @ 0x3C", (char *)"SSD1306 @ 0x3D", (char *)"SH1106 @ 0x3C", (char *)"SH1106 @ 0x3D"};
  int rc;
  rc = oledInit(&ssoled, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, GROVE_SDA_PIN, GROVE_SCL_PIN, RESET_PIN, 800000L); // use standard I2C bus at 400Khz
  if (rc != OLED_NOT_FOUND)
  {
    oledFill(&ssoled, 0, 1);
    //    oledWriteString(&ssoled, 0, 0, 0, msgs[rc], FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0, 0, 0, (char *)"one moment...", FONT_SMALL, 0, 1);
    delay(2000);
  }
  oledSetBackBuffer(&ssoled, ucBackBuffer);

  qdec.begin();
  Wire.begin(); //Join the bus as master.
  // Wire.setClock(3400000);
  pinMode(s1up, INPUT_PULLUP);
  pinMode(s1down, INPUT_PULLUP);
  pinMode(s2up, INPUT_PULLUP);
  pinMode(s2down, INPUT_PULLUP);
  pinMode(push, INPUT);
  pinMode(d1, OUTPUT);
  pinMode(d2, OUTPUT);
  pinMode(d3, OUTPUT);
  pinMode(d4, OUTPUT);
  pinMode(d5, OUTPUT);
  pinMode(fl, INPUT_PULLUP);
  pinMode(fr, INPUT_PULLUP);
  digitalWrite(d1, LOW);
  digitalWrite(d2, LOW);
  digitalWrite(d3, LOW);
  digitalWrite(d4, LOW);
  digitalWrite(d5, LOW);

  EEPROM.get(0, volca);
  EEPROM.get(20, chan);
  EEPROM.get(22, trm);
  EEPROM.get(24, lefty);

  if (volca == 1) digitalWrite(d5, HIGH);
  sendchan();
  disp();
}

void loop() {
  checkSensor();
  //  for (int a = 0; a < 4; a++) {
  doMidi();
  //  }
  ss++;
  if (cw || ccw || sensorLive) ss = 0;
  if (ss > ssdelay) {
    scrnsvr();
  }
  checkSw();
  checkForce();
  liveDisp();
  ther();
}

void ther() {
  if (mode == 3) {
    midiEventPacket_t rx = MidiUSB.read();
    switch (rx.header) {
      case 0:
        break; //No pending events
      case 0x9:
        mode[0] = rx.byte2 % 12;
        break;
      case 0x8:
        break;
    }
  }
}

void checkForce() {

  int tfilter;
  if (volca == 2) {
    tfilter = 6;
  }
  int al;
  int ar;

  if (lefty) {
    ar = analogRead(fl);
    al = analogRead(fr);
  } else {
    al = analogRead(fl);
    ar = analogRead(fr);
  }

  touch1 = (1023 - al) - 466 - damp;
  if (touch1 < 0) {
    touch1 = 0;
  }
  if ((touch1 < tcheck1 - tfilter) || (touch1 > tcheck1 + tfilter)) {
    ss = 0;
    if (touch1 > 511) {
      touch1 = 511;
    }
    if (!sensorLive && volca == 2) {
      oledDrawLine(&ssoled, 0, 32, 0, 32 - touch1 / 16, 1);
      oledDumpBuffer(&ssoled, NULL);
      oledDrawLine(&ssoled, 0, 32, 0, 0, 0);
      oledDumpBuffer(&ssoled, NULL);
    }

    pitchBendChange(chan, mapf(touch1, 0, 511, 8192, 16383));

    tcheck1 = touch1;
  }

  touch2 = (1023 - ar) - 466 - damp;
  if (touch2 < 0) {
    touch2 = 0;
  }
  if ((touch2 < tcheck2 - tfilter) || (touch2 > tcheck2 + tfilter)) {
    ss = 0;
    if (touch2 > 511) {
      touch2 = 511;
    }
    if (!sensorLive && volca == 2) {
      oledDrawLine(&ssoled, 127, 32, 127, 32 - touch2 / 16, 1);
      oledDumpBuffer(&ssoled, NULL);
      oledDrawLine(&ssoled, 127, 32, 127, 0, 0);
      oledDumpBuffer(&ssoled, NULL);
    }
    int tmap = mapf(touch2, 0, 511, 0, 127);
    if (!s1ul) {
      controlChange(chan, 1, tmap);
    } else if (!s1dl) {
      controlChange(chan, cct, tmap);
    } else {
      controlChange(chan, 7, tmap);
    }
    tcheck2 = touch2;
  }

}

void scrnsvr() {
  int tim;
  int sy;
  int sx;
  int siz;
  int moon;
  while (!cw && !ccw && !sensorLive && ss != 0) {
    checkSensor();
    checkSw();
    checkForce();
    if (!tim) {
      oledFill(&ssoled, 0, 1);
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
        if (sx == moon) oledEllipse(&ssoled, sx, sy - random(8) - 8, siz, siz, 1, 1);

        oledSetPixel(&ssoled, sx, sy, 1, 1);

      }
      sx = 0;
      for (int a = 0; a < 18; a = a + 3) {
        oledSetPixel(&ssoled, random(128), random(16), 1, 1);
      }
      oledDumpBuffer(&ssoled, NULL);
    }
    tim++;
    delay(1);
    if (tim > 2000) tim = 0;
  }
  ss = 0;
  disp();
}

void checkSw() {
  bool s1un = digitalRead(s1up);
  bool s1dn = digitalRead(s1down);
  bool s2un = digitalRead(s2up);
  bool s2dn = digitalRead(s2down);
  unpush = digitalRead(push);

  ::SimpleHacks::QDECODER_EVENT event = qdec.update();
  if (event & ::SimpleHacks::QDECODER_EVENT_CW) {
    cw = 1;
  } else if (event & ::SimpleHacks::QDECODER_EVENT_CCW) {
    ccw = 1;
  }

  if (s1un != s1ul) {
    s1ul = s1un;
    disp();
  }
  if (s1dn != s1dl) {
    s1dl = s1dn;
    disp();
  }
  if (!s2un) {
    if (!unpush) {
      controlChange(chan, 80 + (2 * param), 127);
      delay(6);
      controlChange(chan, 80 + (2 * param), 0);
    } else {
      if (param) {
        param--;
      } else {
        param = 3;
      }

    }
    disp();
    while (!digitalRead(s2up)) {}
    //  delay(64);
  }
  if (!s2dn) {
    if (!unpush) {
      controlChange(chan, 81 + (2 * param), 127);
      delay(6);
      controlChange(chan, 81 + (2 * param), 0);
    } else {
      param++;
      if (param > 3) param = 0;
    }
    disp();
    while (!digitalRead(s2down)) {}
    delay(64);
  }

  if (cw) {
    cw = false;
    if (param == 0) {
      if (unpush) {
        chan++;
        if (chan > 15) chan = 0;
        sendchan();
      } else {
        mode++;
        if (mode > 3) mode = 0;
      }
    } //end param 0
    if (param == 1) {
      if (mode == 0) {
        trm ++;
        if (trm > 15) trm = 0;
      } else {
        if (!unpush) {
          vol1 ++;
        } else {
          vol1 += 10;
        }
        if (vol1 > 127) vol1 = 127;
        controlChange(chan, 7, vol1);
      }
    } //end param 1
    if (param == 2) {
      if (mode == 0) {
        if (lefty) {
          lefty = false;
        } else {
          lefty = true;
        }
      }
      if (mode == 1 || mode == 2) {
        if (!unpush) {
          cct += 10;
        } else {
          cct++;
        }
        if (cct > 127) cct = 0;
      } else if (mode == 3) {        //end mode 2/3
        if (!unpush) {
          vel += 10;
        } else {
          vel++;
        }
        if (vel > 127) vel = 0;
      } //end mode 3
    } //end param 2
    if (param == 3) {
      if (mode == 0) {
        if (!unpush) {
          ewrite();
        }
      }
      if (mode == 1 || mode == 2) {
        if (!unpush) {
          ccv ++;
        } else {
          ccv += 10;
        }
        if (ccv > 127) ccv = 127;
        controlChange(chan, cct, ccv);
      } else if (mode == 3) {        //end mode 2/3

        mode[1]++;
        if (mode[1] > 7) mode[1] = 0;

      } //end mode 3
    } //end param 3
    disp();
  } // end cw
  else if (ccw) {
    ccw = false;
    if (param == 0) {
      if (unpush) {
        chan--;
        if (chan > 15) chan = 15;
        sendchan();
      } else {
        if (mode) {
          mode--;
        } else {
          mode = 3;
        }
      }
    } //end param 0
    if (param == 1) {
      if (mode == 0) {
        trm --;
        if (trm > 15) trm = 15;
      } else {
        if (!unpush) {
          vol1--;
        } else {
          if (vol1 == 127) {
            vol1 -= 7;
          } else {
            vol1 -= 10;
          }
        }
        if (vol1 > 127) vol1 = 0;
        controlChange(chan, 7, vol1);
      } //end mode 3
    } //end param 1
    if (param == 2) {
      if (mode == 0) {
        volca--;
        if (volca == 1) {
          digitalWrite(d5, HIGH);
        } else {
          digitalWrite(d5, LOW);
        }
        if (volca > 2)volca = 2;
      }
      if (mode == 1 || mode == 2) {
        if (!unpush) {
          cct -= 10;
          if (cct > 127) cct = 127;
        } else {
          if (cct) {
            cct--;
          } else {
            cct = 127;
          }
        }
      } else if (mode == 3) {        //end mode 2/3
        if (!unpush) {
          vel -= 10;
          if (vel > 127) vel = 127;
        } else {
          if (vel) {
            vel--;
          } else {
            vel = 127;
          }
        }
      } //end mode 3
    } //end param 2
    if (param == 3) {
      if (mode == 0) {
        if (!unpush) {
          ntoff();
        }
      }
      if (mode == 1 || mode == 2) {

        if (!unpush) {
          ccv--;
        } else {
          if (ccv == 127) {
            ccv -= 7;
          } else {
            ccv -= 10;
          }
        }
        if (ccv > 127) ccv = 0;
        controlChange(chan, cct, ccv);
      } else if (mode == 3) {        //end mode 2/3

        mode[1]--;
        if (mode[1] > 7) mode[1] = 7;

      } //end mode 3
    } //end param 3
    disp();
  } // end ccw
  /*  else if (!unpush) {
      for (int unote = 12; unote < 107; unote++) {
        MIDI.send(128, unote, 0, chan); //turn note off
        delay(2);
      }
    }*/
} //end checkSw

void liveDisp() {
  if ((sensorLive) && (mode == 0)) {
    for (int twice = 0; twice < 2; twice++) {
      oledSetPixel(&ssoled, (x / 2), 48 - (y / 4), 1, 1);

      //oledDrawLine(&ssoled, (x / 2), 48 - (y / 4) + 3, (x / 2) + 6, 48 - (y / 4) + 3, 1);
      //oledDrawLine(&ssoled, (x / 2) + 1, 48 - (y / 4) + 2, (x / 2) + 3, 48 - (y / 4) - 3, 1);
      //oledDrawLine(&ssoled, (x / 2) + 4, 48 - (y / 4) - 2, (x / 2) + 5, 48 - (y / 4) + 2, 1);
      oledDumpBuffer(&ssoled, NULL);
    }

    sensorLive = false;
  }
}

void disp() {
  ss = 0;
  oledFill(&ssoled, 0, 1);

  if (mode == 0) {
    /*
      display.setTextSize(3);
      display.setCursor(2, 0);
      display.print("woz.lol");
      display.setTextSize(2);
      display.setCursor(38, 33);
      display.print("control");
      display.setCursor(0, 21);
      display.print("gesture");
      display.setTextSize(0);
    */
    oledWriteString(&ssoled, 0, 0, 0, (char *)"woz.lol", FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0, 0, 1, (char *)"gesture", FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0, 0, 2, (char *)"sensor", FONT_SMALL, 0, 1);

    oledWriteString(&ssoled, 0, 36, 6, (char *)"sens", FONT_SMALL, 0, 1);
    if (trm == 0) {
      oledWriteString(&ssoled, 0, 36, 7, (char *)"full", FONT_SMALL, 0, 1);
    } else if (trm == 15) {
      oledWriteString(&ssoled, 0, 36, 7, (char *)"OFF", FONT_SMALL, 0, 1);
    } else {
      oledWriteString(&ssoled, 0, 36, 7, (char *)"-", FONT_SMALL, 0, 1);
      sprintf(szTemp, "%d", (int)trm);
      oledWriteString(&ssoled, 0, 42, 7, szTemp, FONT_SMALL, 0, 1);
    }

    if (volca == 1) {
      oledWriteString(&ssoled, 0, 68, 7, (char *)"vlca", FONT_SMALL, 0, 1);
    } else if (volca == 2) {
      oledWriteString(&ssoled, 0, 68, 7, (char *)"safe", FONT_SMALL, 0, 1);
    } else {
      oledWriteString(&ssoled, 0, 68, 7, (char *)"norm", FONT_SMALL, 0, 1);
    }
    if (lefty) {
      oledWriteString(&ssoled, 0, 68, 6, (char *)"left", FONT_SMALL, 0, 1);
    } else {
      oledWriteString(&ssoled, 0, 68, 6, (char *)"rght", FONT_SMALL, 0, 1);
    }

    oledWriteString(&ssoled, 0, 98, 6, (char *)"save>", FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0, 98, 7, (char *)"<pnic", FONT_SMALL, 0, 1);

  }

  if (mode == 3) {
    //  display.drawLine(32, 0, 32, 47, SSD1306_WHITE);   // vertical lines
    oledDrawLine(&ssoled, 64, 0, 64, 47, 1);

    //  display.drawLine(96, 0, 96, 47, SSD1306_WHITE);

    oledDrawLine(&ssoled, 1, 24, 126, 24, 1);

    //  for (int a = 0; a < 127; a++) {
    //    display.drawPixel(a, 0, SSD1306_WHITE);     // horizontal lines
    //    display.drawPixel(a, 24, SSD1306_WHITE);
    //    display.drawPixel(a, 47, SSD1306_WHITE);
    //  }
  }
  if (mode == 1) {
    oledEllipse(&ssoled, 63, 24, 23, 23, 1, 0);
    if (!s1ul) {
      oledWriteString(&ssoled, 0, 2, 3, (char *)"-ptch", FONT_SMALL, 0, 1);
    } else {
      oledWriteString(&ssoled, 0, 2, 3, (char *)"mod", FONT_SMALL, 0, 1);
    }
    oledWriteString(&ssoled, 0, 103, 3, (char *)"ptch", FONT_SMALL, 0, 1);
  }
  if (mode == 2) {
    oledEllipse(&ssoled, 63, 24, 5, 5, 1, 0);
    oledDrawLine(&ssoled, 1, 24, 126, 24, 1);
    oledWriteString(&ssoled, 0, 52, 1, (char *)"ptch", FONT_SMALL, 0, 1);
  }
  if (mode) {
    oledWriteString(&ssoled, 0, 36, 6, (char *)"vlme", FONT_SMALL, 0, 1);
    sprintf(szTemp, "%d", (int)vol1);
    oledWriteString(&ssoled, 0, 36, 7, szTemp, FONT_SMALL, 0, 1);
  }
  if (mode == 3) {
    oledWriteString(&ssoled, 0, 72, 6, (char *)"vel", FONT_SMALL, 0, 1);
    sprintf(szTemp, "%d", (int)vel);
    oledWriteString(&ssoled, 0, 72, 7, szTemp, FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0, 105, 6, (char *)"CC=", FONT_SMALL, 0, 1);
    sprintf(szTemp, "%d", (int)ccv);
    oledWriteString(&ssoled, 0, 105, 7, szTemp, FONT_SMALL, 0, 1);
  }

  if (mode == 1 || mode == 2) {
    oledWriteString(&ssoled, 0, 72, 6, (char *)"CC#", FONT_SMALL, 0, 1);
    sprintf(szTemp, "%d", (int)cct);
    oledWriteString(&ssoled, 0, 72, 7, szTemp, FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0, 105, 6, (char *)"CC=", FONT_SMALL, 0, 1);
    sprintf(szTemp, "%d", (int)ccv);
    oledWriteString(&ssoled, 0, 105, 7, szTemp, FONT_SMALL, 0, 1);

    if (!s1ul) {
      oledWriteString(&ssoled, 0, 55 + ((mode - 1) * 49), 1 + (mode - 1), (char *)"mod", FONT_SMALL, 0, 1);
    } else if (!s1dl) {
      oledWriteString(&ssoled, 0, 55 + ((mode - 1) * 49), 1 + (mode - 1), (char *)"CC=", FONT_SMALL, 0, 1);
    } else {
      oledWriteString(&ssoled, 0, 52 + ((mode - 1) * 49), 1 + (mode - 1), (char *)"vlme", FONT_SMALL, 0, 1);
    }

  }// end mode 1/2

  if (mode == 3) {
    if (!s1ul) {

    } else if (!s1dl) {

    } else {

    }
  }  // end mode 3

  // display.setTextSize(0);
  oledWriteString(&ssoled, 0, 7, 6, (char *)"ch", FONT_SMALL, 0, 1);
  sprintf(szTemp, "%d", (int)chan + 1);
  oledWriteString(&ssoled, 0, 7, 7, szTemp, FONT_SMALL, 0, 1);


  switch (param) {
    case 0:
      oledRectangle(&ssoled, 0, 48, 31, 63, 1, 0);
      break;
    case 1:
      oledRectangle(&ssoled, 32, 48, 63, 63, 1, 0);
      break;
    case 2:
      oledRectangle(&ssoled, 64, 48, 95, 63, 1, 0);
      break;
    case 3:
      oledRectangle(&ssoled, 96, 48, 127, 63, 1, 0);
      break;
  }

  oledDumpBuffer(&ssoled, NULL);
}

long mapf(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void checkSensor() {
  if ( zx_sensor.positionAvailable() ) {
    uint8_t xPos = zx_sensor.readX();
    uint8_t zPos = zx_sensor.readZ();
    if (( zPos != ZX_ERROR ) && ( xPos != ZX_ERROR ) && zPos < 192 - (trm * 12) && zPos > trm) {
      sensorLive = true;
      x = xPos;
      y = mapf(zPos, trm, 192 - (trm * 12), 0, 192);
    }
  }
}

void doMidi() {

  if (sensorLive) {

    ccZero++;
    if (ccZero > 2) ccZero = 2;

    if (mode == 1) {
      if (x > 128) {
        if (x > 240) x = 240;
        xxp = mapf(x, 128, 240, 8191, 16383);
        pb();
      }
      if (x < 128) {
        if (x < 5) x = 5;
        int xx = mapf(x, 128, 5, 0, 127);
        if (!s1ul) {
          xxp = mapf(x, 15, 128, 0, 8191);
          pb();
        } else {
          controlChange(chan, 1, xx);
        }
      }
      if (y < 15) y = 15;
      int yy = mapf(y, 15, 192, 127, 0);
      if (analogRead(fr) > 1020) {
        if (!s1ul) {
          controlChange(chan, 1, yy);
        } else if (!s1dl) {
          controlChange(chan, cct, yy);
        } else {
          controlChange(chan, 7, yy);
        }
      }
    }                    // end mode 1

    if (mode == 2) {
      if (x < 5) x = 5;
      if (x > 240) x = 240;
      if (y > 96) y = 96;
      if (y < 15) y = 15;
      xxp = mapf(y, 15, 96, 16383, 8191);
      int xx = mapf(x, 5, 240, 0, 127);
      pb();
      if (analogRead(fr) > 1020) {
        if (!s1ul) {
          controlChange(chan, 1, xx);
        } else if (!s1dl) {
          controlChange(chan, cct, xx);
        } else {
          controlChange(chan, 7, xx);
        }
      }
    }

    if (mode == 3) {
      if (x > 128) {
        if (x > 240) x = 240;
        int xx = mapf(x, 128, 240, 0, 127);
        xxp = mapf(x, 128, 240, 8191, 16383);
        if (!s1ul) {
          //cc3
        } else if (!s1dl) {
          //cc9
        } else {
          //cc6
        }
      }
      if (x < 128) {
        if (x < 5) x = 5;
        int xx = mapf(x, 128, 15, 0, 127);
        xxp = mapf(x, 5, 128, 0, 8191);
        if (!s1ul) {
          //cc1
        } else if (!s1dl) {
          //cc7
        } else {
          //cc4
        }
      }
      if (y < 15) y = 15;
      if (y > 192) y = 192;
      int yy = mapf(y, 15, 192, 127, 0);
      xxp = mapf(y, 15, 96, 8191, 16383);
      if (!s1ul) {
        //cc2
      } else if (!s1dl) {
        //cc8
      } else {
        //cc5
      }
    }                    // end mode 3

    //  delay(4);

  } else {     // sensor off live, zero it out
    if (ccZero) {
      ccZero--;
      pitchBendChange(chan, 8191);
      if (mode == 1 || mode == 2) {
        controlChange(chan, 1, 0);
        controlChange(chan, 7, vol1);
        controlChange(chan, cct, ccv);
      } else {
        //zero out mode 3 and panic notes

      }
      //    delay(4);
    }
  }
}

void pb() {
  if (analogRead(fl) > 1020) {
    pitchBendChange(chan, xxp);
  }
}

void ewrite() {

  while (!digitalRead(push)) {
    int wait;
    wait++;
    if (wait > 1600) {
      rst();
      break;
    }
    delay(1);
  }

  EEPROM.put(0, volca);
  EEPROM.put(20, chan);
  EEPROM.put(22, trm);
  EEPROM.put(24, lefty);
  oledWriteString(&ssoled, 0, 92, 6, (char *)" saved ", FONT_SMALL, 0, 1);
  oledDumpBuffer(&ssoled, NULL);
  delay(600);
}

void rst() {
  vol1 = 96;
  cct = 70;
  ccv = 0;

  volca = 0;
  digitalWrite(d5, LOW);
  chan = 1;
  trm = 0;
  lefty = 0;
  sendchan();
  oledWriteString(&ssoled, 0, 92, 6, (char *)" reset ", FONT_SMALL, 0, 1);
  oledDumpBuffer(&ssoled, NULL);
  delay(800);
}

void ntoff() {

  oledWriteString(&ssoled, 0, 92, 7, (char *)" ntoff ", FONT_SMALL, 0, 1);
  oledDumpBuffer(&ssoled, NULL);
  for (int unote = 0; unote < 127; unote++) {
    noteUSB(chan, unote, 0);
    delay(4);
  }
  MidiUSB.flush();
  delay(200);

}

void sendchan() {
  digitalWrite(d4, HIGH && (chan & B00001000));
  digitalWrite(d3, HIGH && (chan & B00000100));
  digitalWrite(d2, HIGH && (chan & B00000010));
  digitalWrite(d1, HIGH && (chan & B00000001));
}
