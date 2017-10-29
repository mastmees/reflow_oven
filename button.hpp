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
#ifndef __button_hpp__
#define __button_hpp__

#include <avr/io.h>

class Button
{
  uint8_t state,clicks;
public:
  Button()
  {
    state=0xff;
    clicks=0;
  }
  
  // updates the button with read state which is zero or nonzero value
  // buttons read 1 when not pressed
  void Update(uint8_t b)
  {
    state=(state<<1)|(b?1:0);
    if (state==0xf0)
      clicks++;
  }
  
  // read a button click. 1 means clicked, 0 not clicked
  uint8_t Read()
  {
    if (clicks) {
      clicks--;
      return 1;
    }
    return 0;
  }
  
  // forget any accumulated button clicks
  void Clear()
  {
    clicks=0;
  }
  
  // check if the button is being held down
  uint8_t Pressed()
  {
    return (state==0);
  }
  
};

#endif
