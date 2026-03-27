ARP & MIDI ROUTER
using the RP2040 & a promicro 
by Joseph Wozniak

arduino project. a midi router, arpeggiator, processor, multitool. the goal is to determine the pins to use then create the rp2040 code.  hardware:a pro micro has generic usb midi to din midi code only. it has din midi data lines with no optoisolator, just a resistor voltage divider, to the din mini in pins we choose on an RP2040 zero which is the main brain. it will foundationally have community code that hosts 1 to 4 usb midi controllers from a wired in usb 2 port that may optionally have a hub attached. we also have a rotary encoder with 5 pins, 2 for the push click. we have a SSD1306  128x64 monochrome with blue top 3/4 and yellow bottom 1/4. on the same i2c channel we will also have a VL53L0X with different ways for it to control parameters or send notes/cc. we will also have two pins that are for two foot pedals, when either is stepped on, it's pin is pulled low, otherwise internally pulled high. try to avoid using pin 11. look through all the code in the examples folder in this folder. start with giving me the pin numbers as they appear on the rp2040 zero as they are marked on the board for each component connection. then create the new rp2040 code as arpnmidi.txt in this folder, not an ino file so I can copy it into Arduino without losing my board settings which get reset when opening a new ino, and project folder stuff for arduino gets weird.

firmware:
receives midi beat clock F8 messages for auto BPM setting, 120 default, able to set in menu
screen needs to update in real time if parameters are changing and the menu system must be non-blocking so that midi flows while we can change things. if a parameter changes the flow of live midi, like channel settings for example, we need to send an all note off before changing the setting and allowing new notes. things like arp velocity wouldn't need this. we only update the screen if something changed on it.
the yellow 1/4 on the bottom will be where we see the name of the setting, these need to start with a number too. above in the blue we will se the parameter, as big as fits, or sometimes the parameter will be smaller text to fit a graphic representation. settings need to persist, with a simple way to switch presets as a menu parameter, explained below.

* arp timing is only restart-style, new restart with new set of notes held, reset when all keys up

menu settings - a single click and release, while not turning, of the encoder, toggles the menu between two states: either turning the knob changes which setting is on deck, or turning the knob changes the parameter of that setting. On settings with more that a couple dozen parameter values, we want to increment by +/-10 if we are holding the knob while turning, and not also toggle back to settings.
name on screen (notes not for the screen)
1. bpm (can set it manually, but its taken over if something is sending bpm on the midi, din midi bpm takes priority, big numbers in the upper blue part)
2. arp mode (up, down, up‑down 1, up‑down 2, trigger, random, off. show also a symbolic representation of the mode with line art in the blue section)
3. division (arp beat divisions, 1/1 1/2 1/2T (triplet half) 1/4 1/4T 1/8 1/8T 1/16 1/16T 1/32 1/32T 1/64. show also a pie chart in the blue section showing this fraction portion of the pie.)
4. velocity (for arp notes. also show the blue of the screen get more full from left to right as vel increases to 100%, but just the top 1/2 of the screen so we can see number on the row below this bar but still in the blue)
5. length (note length, show as duty cycle percentage similar to the way we see velocity)
6. pattern (see notes below, show a simple set of circles on the blue part of the screen to show what the pattern looks like, as far as notes and rests, sort of like morse code, not too long, just enough reps of it to get the idea)
7. input ch (for data to go to the three internal output channels arp, bass, passthrough)
8. arp out ch (off or any ch. if bass or through are set to the same ch, arp operations takes priority)
9. bass ch (settable 1 to 3 octaves below input. only sends the lowest note if more than one note is down on the input ch. if a new note comes that is lower, on this ch we release the last note or all notes and then send this note, but any higher notes are rejected until all notes are up then we can start over with the next note being allowed but then only ones that are lower unless its let up. and if the lowest note is let up and there is still a higher one from earlier still held, we then go to that one, so basically its like a mono synth with lowest note priority. does not work if arp is set to the same ch. parameters go: 0.off 1.ch1.-1oct 2.ch1.-2oct 3.ch1.-3oct 4.ch2.-1oct 5.ch2.-2oct 6.ch2.-3oct 7.ch3.-1oct etc)
10. through out ch (if this passthrough ch is set to the same channel as arp or bass, arp takes priority)
11. remote channel (this is just for the midi triggered by the 2 external pedals)
12. sensor channel (if the distance sensor is set to send notes or cc, it will be on this ch. if the sensor is doing notes, these also go through the note scale filter if its on)
13. sensor mode (notes below, simple line art or text symbol ways to represent each mode)
14. force key (off or settable to one of the 12 notes. I have old code for this in the same folder, which also uses the same screen and shows what to show)
15. force scale (sets it to one of the scales in the example in this folder for scale forcing)
16. guitar/piano (just shows the key and scale as dots on a guitar frettboard, with the active notes going into input ch indicated similarly. or, it shows a 1oct piano similar to the example code)
17. remote 1 (note to send just once with debounce delay, on then off quickly, or cc to max min similarly, when pedal one's pin is pulled low. scroll through all midi notes and cc channels, skip by 10 if holding the knob instead of switching back to settings change mode)
18. remote 2 (for second input)
19. load preset (experience the settings change live as we scroll though presets in real time)
20. save preset (save to one of 16 slots, show them in a grid of 4x4 squares on the top blue part as well as the number, same with load screen)
21. screen saver (show the screen saver that also comes up if no notes or cc go through the device (ignoring beat/bpm data) within 1min. it looks like a mountain with a moon and stars that is procedurally generated and this is also in some of the example scripts in this folder)
22. PANIC (in this setting, click the nob to panic (send all midi offs) and it will go right back to settings change mode, not staying toggled to change the parameter like the others. also do panic any time we hold the knob down for two seconds at any time without turning it)

turn to change setting, push to switch between setting choosing and parameter changing.

channels passthrough in to out unless its the channel that is set as input ch.

in can be from any device and out is to all devices (except we don't want out to go back to the device it came from to avoid a feedback loop. but outputs from this master device (arp/bass/passthorough) can go back to the input device.) 

Arp patterns (per‑step note selection) based on the korg nanokey fold
Here “pattern” = which note (or chord behavior) is played on each of the 16 steps, expressed as an index into the currently held notes (N0 = lowest, N1 = next, etc.). Example pattern list:
1. Up 1‑oct range: [N0, N1, N2, N3, N0, N1, N2, N3, …]
2. Down: [N3, N2, N1, N0, …]
3. Up‑down 1 (no repeat ends): [0,1,2,3,2,1,0,1,2,3,2,1,…]
4. Up‑down 2 (repeat ends): [0,1,2,3,3,2,1,0,0,1,2,3,…]
5. Random single note: each step picks random of [0..max]
6. Trigger chord (Korg “Trigger” behavior): [ALL, ALL, ALL, …] – play all held notes each step.
7. 16‑step “rhythm” style pattern (use rests): [0, REST, 0, REST, 1, REST, 1, REST, 0, REST, 2, REST, 0, REST, 3, REST]
8. 3‑note ostinato: [0, 1, 2, 0, 1, 2, …]
9. Octave walk: [0@oct0, 0@oct1, 0@oct2, 0@oct3, 1@oct0, 1@oct1, …]
10. Fifth‑based: [0, 0+7, 1, 1+7, 2, 2+7, …]
11. “Bass then chord”: [0, ALL, 0, ALL, 1, ALL, 1, ALL, …]
12. “Chord then run”: [ALL, 0,1,2,3, ALL, 3,2,1,0, …]
Internally, each pattern is a 16‑element table; each element is either a note index (possibly with an octave offset) or a special symbol like REST or ALL.

distance sensor modes
1. param+2 (in this mode, whatever setting is on screen, the parameter is increased by 1 if we are in the first half of the range of the sensor, and 2 increments if in the closer half, and it returns to its set value when leaving the sensor area, and this parameter change is not saved or persisted, and some parameters may not make sense to do this and they can be exempt. this is only for arp mode, division, velocity, length, output ch, force key, force scale.) 
2. param-2 (like +2 but it decrements instead)
3. param+3 (uses thirds of the sensor range for up to 3 increments, be careful to always mind rollover from max value to min and vice versa)
4. param-3
5. param-100 (takes over the parameter and sets it to 0-100 percent of its possible values based on 100% being closest to the sensor and 0 or off if all the way away. does not save to preset or persist, returns to saved value if we move to another setting while in this mode.
6. pitch up (sends pitch shift up values 0-100%, 100 being closest to the sensor)
7. pitch down
8. notes C0 (distance of 0-100% goes between sending 13 notes, starting with C0. we have example code for this sensor sending notes, but it may not do 13 notes but we need an odd number so it can end at the note it starts with nicely. the note length is set by the arp note length)
9. notes C1
10. notes C2
11. notes C3
12. notes C4
13. notes C5
14. notes C6
15. notes C7
16. cc 1 (0-100% on this cc#)
17. cc 2
18. cc 3...19
19. loop trigger (if something is seen in the sensor range within the first 3/4 of its range, we send not 103 on then off very quickly, to trigger the 1010 balckbox. if it detects within the closest 1/4 of its range, we instead do this with note number 104. )



• Use these RP2040 Zero board-marked pins:

  - 0 DIN MIDI IN
  - 1 DIN MIDI OUT
  - 2 I2C SDA
  - 3 I2C SCL
  - 6 encoder A
  - 7 encoder B
  - 8 encoder push
  - 9 foot pedal 1
  - 10 foot pedal 2
  - 14 USB host D+
  - 15 USB host D-

  GP11 is avoided.
