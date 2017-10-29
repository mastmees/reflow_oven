/*
MIT License

Copyright (c) 2017 Madis Kaal

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __servo_hpp__
#define __servo_hpp__

#include <avr/io.h>
#include "serial.hpp"

extern Serial serial;

// Pulse() needs to be called every 20ms to run the servo
// this code limits the physical change rate to keep the motor current
// consumption in sane range
//
class Servo
{
  uint8_t position;
  
public:
  Servo()
  {
    TCCR2B=0;    // stop counter by disconnecting clock
    TCNT2=0;     // start counting at bottom
    OCR2A=0;     // set compare value to 0, this is where the counter resets
    TCCR2A=0x33; // fast PWM mode 7, output on OC2B, set at compare match
                 // clear at bottom
    TCCR2B=0x0c;
    position=185; // about middle
    OCR2B=255-position-1;
  }

  uint8_t GetPosition(void)
  {
    return 255-OCR2B-1;
  }
  
  void SetRealPosition(uint8_t pos)
  {
    OCR2B=255-pos-1;
  }
  
  void SetPosition(uint8_t pos)
  {
    position=pos;
  }

  // servo position change is rate limited here
  // to avoid large current spikes
  void Pulse()
  {
    TCNT2=OCR2B-1;
    uint8_t rp=GetPosition();
    if (rp<position) {
      rp+=3;
      if (rp>position)
        rp=position;
      SetRealPosition(rp);
    }
    if (rp>position) {
      rp-=3;
      if (rp<position)
        rp=position;
      SetRealPosition(rp);
    }
  }
  
};

#endif
