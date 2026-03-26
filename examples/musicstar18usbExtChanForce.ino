/*
   musicstar keyboard
  woz.lol zx sensor midi filter
  help from Arnaud Fenioux
  Note to pitch enabled for Korg Volca Sample Copyright 2015 Mauricio Maisterrena
  this version sets 5 pins to control a separate pro mini that filters
  the midi data from the keyboard to set it to the same channel or set it
  to volca sample mode
  WORKS

*/
#include <EEPROM.h>
#include <MIDIUSB.h>
#include <Rotary.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <ZX_Sensor.h>
#include <SPI.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Rotary r = Rotary(8, 9);
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
#define damp 64 // sensitivity adjustment for touch force sensors. lower number is MORE sensitive
#define ssdelay 24000   // screen saver delay

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
byte mode = 1;
int xxp;
int vv;
int ss; // screen saver time

int tcheck1;
int tcheck2;
int touch1;
int touch2;

byte vol1 = 96;
byte cct = 70;
byte ccv = 0;

byte cc1 = 128;
byte cc2 = 1;
byte cc3 = 128;
byte cc4 = 12;
byte cc5 = 11;
byte cc6 = 13;
byte cc7 = 2;
byte cc8 = 3;
byte cc9 = 4;
int volca;
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

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void setup() {
  delay(500);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000);
  Wire.begin(); //Join the bus as master.
  Wire.setClock(3400000);
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
  display.setRotation(2);
  display.clearDisplay();
  display.setTextColor(WHITE, BLACK);
  PCICR |= (1 << PCIE0);
  PCMSK0 |= (1 << PCINT4) | (1 << PCINT5);
  sei();

  EEPROM.get(0, volca);
  EEPROM.get(2, cc1);
  EEPROM.get(4, cc2);
  EEPROM.get(6, cc3);
  EEPROM.get(8, cc4);
  EEPROM.get(10, cc5);
  EEPROM.get(12, cc6);
  EEPROM.get(14, cc7);
  EEPROM.get(16, cc8);
  EEPROM.get(18, cc9);
  EEPROM.get(20, chan);
  EEPROM.get(22, trm);

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
}

void checkForce() {

  int tfilter;
  if (volca == 2) {
    tfilter = 6;
  }

  touch1 = (1023 - analogRead(fl)) - 466 - damp;
  if (touch1 < 0) {
    touch1 = 0;
  }
  if ((touch1 < tcheck1 - tfilter) || (touch1 > tcheck1 + tfilter)) {
    ss = 0;
    if (touch1 > 511) {
      touch1 = 511;
    }
    if (!sensorLive && volca == 2) {
      display.drawLine(0, 32, 0, 32 - touch1 / 16, SSD1306_WHITE);
      display.display();        // transfer internal memory to the display
      display.drawLine(0, 32, 0, 0, SSD1306_BLACK);
      display.display();        // transfer internal memory to the display
    }
    if (!s1ul) {
      controlChange(chan, 1, mapf(touch1, 0, 511, 0, 127));
    } else {
      pitchBendChange(chan, mapf(touch1, 0, 511, 8192, 16383));
    }

    tcheck1 = touch1;
  }

  touch2 = (1023 - analogRead(fr)) - 466 - damp;
  if (touch2 < 0) {
    touch2 = 0;
  }
  if ((touch2 < tcheck2 - tfilter) || (touch2 > tcheck2 + tfilter)) {
    ss = 0;
    if (touch2 > 511) {
      touch2 = 511;
    }
    if (!sensorLive && volca == 2) {
      display.drawLine(127, 32, 127, 32 - touch2 / 16, SSD1306_WHITE);
      display.display();        // transfer internal memory to the display
      display.drawLine(127, 32, 127, 0, SSD1306_BLACK);
      display.display();        // transfer internal memory to the display
    }
    int tmap = mapf(touch2, 0, 511, 0, 127);
    if (!s1ul) {
      pitchBendChange(chan, mapf(touch2, 0, 511, 8192, 16383));
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
  ss = 0;
  disp();
}

void checkSw() {
  bool s1un = digitalRead(s1up);
  bool s1dn = digitalRead(s1down);
  bool s2un = digitalRead(s2up);
  bool s2dn = digitalRead(s2down);
  unpush = digitalRead(push);
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
      } else if (mode == 1 || mode == 2) {
        if (!unpush) {
          vol1 ++;
        } else {
          vol1 += 10;
        }
        if (vol1 > 127) vol1 = 127;
        controlChange(chan, 7, vol1);
      } else if (mode == 3) {        //end mode 2/3
        if (!s1ul) {
          if (unpush) {
            cc1 ++;
          } else {
            cc1 += 10;
          }
          if (cc1 > 128) cc1 = 0;
        } else if (!s1dl) {
          if (unpush) {
            cc7 ++;
          } else {
            cc7 += 10;
          }
          if (cc7 > 128) cc7 = 0;
        } else {
          if (unpush) {
            cc4 ++;
          } else {
            cc4 += 10;
          }
          if (cc4 > 128) cc4 = 0;
        }
      } //end mode 3
    } //end param 1
    if (param == 2) {
      if (mode == 0) {
        volca++;
        if (volca == 1) {
          digitalWrite(d5, HIGH);
        } else {
          digitalWrite(d5, LOW);
        }
        if (volca > 2)volca = 0;
      }
      if (mode == 1 || mode == 2) {
        if (!unpush) {
          cct += 10;
        } else {
          cct++;
        }
        if (cct > 127) cct = 0;
      } else if (mode == 3) {        //end mode 2/3
        if (!s1ul) {
          if (unpush) {
            cc2 ++;
          } else {
            cc2 += 10;
          }
          if (cc2 > 128) cc2 = 0;
        } else if (!s1dl) {
          if (unpush) {
            cc8 ++;
          } else {
            cc8 += 10;
          }
          if (cc8 > 128) cc8 = 0;
        } else {
          if (unpush) {
            cc5 ++;
          } else {
            cc5 += 10;
          }
          if (cc5 > 128) cc5 = 0;
        }
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
        if (!s1ul) {
          if (unpush) {
            cc3 ++;
          } else {
            cc3 += 10;
          }
          if (cc3 > 128) cc3 = 0;
        } else if (!s1dl) {
          if (unpush) {
            cc9 ++;
          } else {
            cc9 += 10;
          }
          if (cc9 > 128) cc9 = 0;
        } else {
          if (unpush) {
            cc6 ++;
          } else {
            cc6 += 10;
          }
          if (cc6 > 128) cc6 = 0;
        }
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
      } else if (mode == 1 || mode == 2) {
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
      } else if (mode == 3) {        //end mode 2/3
        if (!s1ul) {
          if (unpush) {
            cc1--;
          } else {
            if (cc1 == 128) {
              cc1 -= 8;
            } else {
              cc1 -= 10;
            }
          }
          if (cc1 > 128 ) cc1 = 128;
        } else if (!s1dl) {
          if (unpush) {
            cc7--;
          } else {
            if (cc7 <= 128) {
              cc7 -= 8;
            } else {
              cc7 -= 10;
            }
          }
          if (cc7 > 128) cc7 = 128;
        } else {
          if (unpush) {
            cc4--;
          } else {
            if (cc4 <= 128) {
              cc4 -= 8;
            } else {
              cc4 -= 10;
            }
          }
          if (cc4 > 128) cc4 = 128;
        }
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
        if (volca < 0)volca = 2;
      }
      if (mode == 1 || mode == 2) {
        if (!unpush) {
          cct -= 10;
          if (cct > 119) cct = 119;
        } else {
          if (cct) {
            cct--;
          } else {
            cct = 119;
          }
        }
      } else if (mode == 3) {        //end mode 2/3
        if (!s1ul) {
          if (unpush) {
            cc2--;
          } else {
            if (cc2 == 128) {
              cc2 -= 8;
            } else {
              cc2 -= 10;
            }
          }
          if (cc2 > 128 ) cc2 = 128;
        } else if (!s1dl) {
          if (unpush) {
            cc8--;
          } else {
            if (cc8 <= 128) {
              cc8 -= 8;
            } else {
              cc8 -= 10;
            }
          }
          if (cc8 > 128) cc8 = 128;
        } else {
          if (unpush) {
            cc5--;
          } else {
            if (cc5 <= 128) {
              cc5 -= 8;
            } else {
              cc5 -= 10;
            }
          }
          if (cc5 > 128) cc5 = 128;
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
        if (!s1ul) {
          if (unpush) {
            cc3--;
          } else {
            if (cc3 == 128) {
              cc3 -= 8;
            } else {
              cc3 -= 10;
            }
          }
          if (cc3 > 128 ) cc3 = 128;
        } else if (!s1dl) {
          if (unpush) {
            cc9--;
          } else {
            if (cc9 <= 128) {
              cc9 -= 8;
            } else {
              cc9 -= 10;
            }
          }
          if (cc9 > 128) cc9 = 128;
        } else {
          if (unpush) {
            cc6--;
          } else {
            if (cc6 <= 128) {
              cc6 -= 8;
            } else {
              cc6 -= 10;
            }
          }
          if (cc6 > 128) cc6 = 128;
        }
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

ISR(PCINT0_vect) {
  unsigned char result = r.process();
  if (result == DIR_CW) {
    cw = true;
  } else if (result == DIR_CCW) {
    ccw = true;
  }
}

void liveDisp() {
  if (sensorLive) {
    // if (mode == 0) {
    display.drawLine((x / 2), 48 - (y / 4) + 3, (x / 2) + 6, 48 - (y / 4) + 3, SSD1306_INVERSE);
    display.drawLine((x / 2) + 1, 48 - (y / 4) + 2, (x / 2) + 3, 48 - (y / 4) - 3, SSD1306_INVERSE);
    display.drawLine((x / 2) + 4, 48 - (y / 4) - 2, (x / 2) + 5, 48 - (y / 4) + 2, SSD1306_INVERSE);
    display.display();        // transfer internal memory to the display
    display.drawLine((x / 2), 48 - (y / 4) + 3, (x / 2) + 6, 48 - (y / 4) + 3, SSD1306_INVERSE);
    display.drawLine((x / 2) + 1, 48 - (y / 4) + 2, (x / 2) + 3, 48 - (y / 4) - 3, SSD1306_INVERSE);
    display.drawLine((x / 2) + 4, 48 - (y / 4) - 2, (x / 2) + 5, 48 - (y / 4) + 2, SSD1306_INVERSE);
    display.display();        // transfer internal memory to the display
    //   }
    sensorLive = false;
  }
}

void disp() {
  ss = 0;
  display.clearDisplay();        // clear the internal memory
  display.setTextSize(0);

  if (mode == 0) {
 /*   display.setTextSize(3);
    display.setCursor(2, 0);
    display.print("woz.lol");
    display.setTextSize(2);
    display.setCursor(38, 33);
    display.print("control");
    display.setCursor(0, 21);
    display.print("gesture");
    display.setTextSize(0);
*/
    display.setCursor(36, 48);
    display.print("sens");
    display.setCursor(39, 56);
    if (trm == 0) {
      display.setCursor(36, 56);
      display.print("full");
    } else if (trm == 15) {
      display.print("OFF");
    } else {
      display.print("-");
      display.print(trm);
    }

    if (volca == 1) {
      display.setCursor(68, 52);
      display.print("vlca");
    } else if (volca == 2) {
      display.setCursor(68, 52);
      display.print("slow");
    } else {
      display.setCursor(68, 52);
      display.print("norm");
    }

    display.setCursor(98, 48);
    display.print("save>");
    display.setCursor(98, 56);
    display.print("<pnic");

  }

  if (mode == 3) {
  //  display.drawLine(32, 0, 32, 47, SSD1306_WHITE);   // vertical lines
    display.drawLine(64, 0, 64, 47, SSD1306_WHITE);
  //  display.drawLine(96, 0, 96, 47, SSD1306_WHITE);

    display.drawLine(1, 24, 126, 24, SSD1306_WHITE);

  //  for (int a = 0; a < 127; a++) {
  //    display.drawPixel(a, 0, SSD1306_WHITE);     // horizontal lines
  //    display.drawPixel(a, 24, SSD1306_WHITE);
  //    display.drawPixel(a, 47, SSD1306_WHITE);
  //  }
  }
  if (mode == 1) {
    display.drawCircle(63, 24, 23, SSD1306_WHITE);
    if (!s1ul) {
      display.setCursor(2, 17);
      display.print("-ptch");
    } else {
      display.setCursor(2, 17);
      display.print("mod");
    }
    display.setCursor(103, 17);
    display.print("ptch");
  }
  if (mode == 2) {
    display.drawCircle(63, 24, 5, SSD1306_WHITE);
    display.drawLine(1, 24, 126, 24, SSD1306_WHITE);
    display.setCursor(52, 6);
    display.print("ptch");
  }
  if (mode == 1 || mode == 2) {
    display.setCursor(36, 48);
    display.print("vlme");
    display.setCursor(39, 56);
    display.print(vol1);
    display.setCursor(72, 48);
    display.print("CC#");
    display.setCursor(72, 56);
    display.print(cct);
    display.setCursor(105, 48);
    display.print("CC=");
    display.setCursor(105, 56);
    display.print(ccv);

    if (!s1ul) {
      display.setCursor(55 + ((mode - 1) * 49), 6 + ((mode - 1) * 6));
      display.print("mod");
    } else if (!s1dl) {
      display.setCursor(55 + ((mode - 1) * 49), 6 + ((mode - 1) * 6));
      display.print("CC=");
    } else {
      display.setCursor(52 + ((mode - 1) * 49), 6 + ((mode - 1) * 6));
      display.print("vlme");
    }

  }// end mode 1/2

  if (mode == 3) {
    if (!s1ul) {
      display.setCursor(39, 48);
      display.print("CC1");
      if (cc1 == 128) {
        display.setCursor(36, 56);
        display.print("ptch");
      } else {
        display.setCursor(39, 56);
        display.print(cc1);
      }
      display.setCursor(72, 48);
      display.print("CC2");
      if (cc2 == 128) {
        display.setCursor(69, 56);
        display.print("ptch");
      } else {
        display.setCursor(72, 56);
        display.print(cc2);
      }
      display.setCursor(104, 48);
      display.print("CC3");
      if (cc3 == 128) {
        display.setCursor(101, 56);
        display.print("ptch");
      } else {
        display.setCursor(104, 56);
        display.print(cc3);
      }
    } else if (!s1dl) {
      display.setCursor(39, 48);
      display.print("CC7");
      if (cc7 == 128) {
        display.setCursor(36, 56);
        display.print("ptch");
      } else {
        display.setCursor(39, 56);
        display.print(cc7);
      }
      display.setCursor(72, 48);
      display.print("CC8");
      if (cc8 == 128) {
        display.setCursor(69, 56);
        display.print("ptch");
      } else {
        display.setCursor(72, 56);
        display.print(cc8);
      }
      display.setCursor(104, 48);
      display.print("CC9");
      if (cc9 == 128) {
        display.setCursor(101, 56);
        display.print("ptch");
      } else {
        display.setCursor(104, 56);
        display.print(cc9);
      }
    } else {
      display.setCursor(39, 48);
      display.print("CC4");
      if (cc4 == 128) {
        display.setCursor(36, 56);
        display.print("ptch");
      } else {
        display.setCursor(39, 56);
        display.print(cc4);
      }
      display.setCursor(72, 48);
      display.print("CC5");
      if (cc5 == 128) {
        display.setCursor(69, 56);
        display.print("ptch");
      } else {
        display.setCursor(72, 56);
        display.print(cc5);
      }
      display.setCursor(104, 48);
      display.print("CC6");
      if (cc6 == 128) {
        display.setCursor(101, 56);
        display.print("ptch");
      } else {
        display.setCursor(104, 56);
        display.print(cc6);
      }
    }
  }  // end mode 3

  display.setTextSize(0);
  display.setCursor(7, 48);
  display.print("ch");
  display.setCursor(7, 56);
  display.print(chan + 1);

  switch (param) {
    case 0:
      display.fillRect(0, 48, 26, 56, SSD1306_INVERSE);
      break;
    case 1:
      display.fillRect(32, 48, 32, 56, SSD1306_INVERSE);
      break;
    case 2:
      display.fillRect(64, 48, 32, 56, SSD1306_INVERSE);
      break;
    case 3:
      display.fillRect(96, 48, 32, 56, SSD1306_INVERSE);
      break;
  }

  display.display();        // transfer internal memory to the display
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
          if (cc3 == 128) {
            pb();
          } else {
            controlChange(chan, cc9, xx);
          }
        } else if (!s1dl) {
          if (cc9 == 128) {
            pb();
          } else {
            controlChange(chan, cc9, xx);
          }
        }
        else {
          if (cc6 == 128) {
            pb();
          } else {
            controlChange(chan, cc6, xx);
          }
        }
      }
      if (x < 128) {
        if (x < 5) x = 5;
        int xx = mapf(x, 128, 15, 0, 127);
        xxp = mapf(x, 5, 128, 0, 8191);
        if (!s1ul) {
          if (cc1 == 128) {
            pb();
          } else {
            controlChange(chan, cc1, xx);
          }
        } else if (!s1dl) {
          if (cc7 == 128) {
            pb();
          } else {
            controlChange(chan, cc7, xx);
          }
        }
        else {
          if (cc4 == 128) {
            pb();
          } else {
            controlChange(chan, cc4, xx);
          }
        }
      }
      if (y < 15) y = 15;
      if (y > 192) y = 192;
      int yy = mapf(y, 15, 192, 127, 0);
      xxp = mapf(y, 15, 96, 8191, 16383);
      if (!s1ul) {
        if (cc2 == 128) {
          pb();
        } else {
          controlChange(chan, cc2, yy);
        }
      } else if (!s1dl) {
        if (cc8 == 128) {
          pb();
        } else {
          controlChange(chan, cc8, yy);
        }
      }
      else {
        if (cc5 == 128) {
          pb();
        } else {
          controlChange(chan, cc5, yy);
        }
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
        if (!s1ul) {
          controlChange(chan, cc1, 0);
          controlChange(chan, cc2, 0);
          controlChange(chan, cc3, 0);
        } else if (!s1dl) {
          controlChange(chan, cc7, 0);
          controlChange(chan, cc8, 0);
          controlChange(chan, cc9, 0);
        } else {
          controlChange(chan, cc4, 0);
          controlChange(chan, cc5, 0);
          controlChange(chan, cc6, 0);
        }
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
  EEPROM.put(2, cc1);
  EEPROM.put(4, cc2);
  EEPROM.put(6, cc3);
  EEPROM.put(8, cc4);
  EEPROM.put(10, cc5);
  EEPROM.put(12, cc6);
  EEPROM.put(14, cc7);
  EEPROM.put(16, cc8);
  EEPROM.put(18, cc9);
  EEPROM.put(20, chan);
  EEPROM.put(22, trm);
  display.setCursor(92, 48);
  display.print(" saved ");
  display.display();        // transfer internal memory to the display
  delay(600);
}

void rst() {
  vol1 = 96;
  cct = 70;
  ccv = 0;

  volca = 0;
  digitalWrite(d5, LOW);
  cc1 = 128;
  cc2 = 1;
  cc3 = 128;
  cc4 = 12;
  cc5 = 11;
  cc6 = 13;
  cc7 = 2;
  cc8 = 3;
  cc9 = 4;
  chan = 1;
  trm = 0;
  sendchan();
  display.setCursor(92, 48);
  display.print(" reset ");
  display.display();        // transfer internal memory to the display
  delay(800);
}

void ntoff() {

  display.setCursor(92, 56);
  display.print(" ntoff ");
  display.display();        // transfer internal memory to the display
  for (int unote = 0; unote < 127; unote++) {
    noteOn(chan, unote, 0);
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
