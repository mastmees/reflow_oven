/* The MIT License (MIT)
 
  Copyright (c) 2017 Madis Kaal <mast@nomad.ee>
 
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
#ifndef __temperaturesensor_hpp__
#define __temperaturesensor_hpp__

#include <avr/io.h>
#include <util/delay.h>
#include "settings.hpp"

#ifndef COUNTOF
 #define COUNTOF(x) (sizeof(x)/sizeof(x[0]))
#endif

// MAX6675 thermocouple interface with moving average filtering
//
class TemperatureSensor
{
uint16_t reading,avg;
uint16_t queue[4]; // adjust the size of moving average length
uint8_t ptr;
float temperature;

  #define CS_LOW() (PORTB&=(~_BV(PB0)))
  #define CS_HIGH() (PORTB|=_BV(PB0))
  #define CLK_LOW() (PORTB&=(~_BV(PB5)))
  #define CLK_HIGH() (PORTB|=_BV(PB5))
  #define GET_BIT() ((PINB>>2)&1)

public:

  TemperatureSensor()
  {
    reading=0;
    for (ptr=0;ptr<COUNTOF(queue);ptr++)
      queue[ptr]=0;
    ptr=0;
    avg=0;
    temperature=0.0;
  }
    
  // MAX6675 has a conversion time of up to 220mS. reading the value
  // aborts any ongoing conversion, so the read rate must be limited
  // and RawRead() must not be called more frequently than every 220mS
  //
  uint16_t RawRead()
  {
    uint16_t v=0;
    uint8_t c;
    CLK_LOW();
    CS_LOW();
    for (c=0;c<16;c++) {
      CLK_HIGH();
      v=(v<<1)|GET_BIT();
      CLK_LOW();
    }
    CS_HIGH();
    reading=v;
    v>>=3;
    avg-=queue[ptr];
    queue[ptr]=v;
    avg+=v;
    ptr=(ptr+1)%COUNTOF(queue);
    temperature=avg/COUNTOF(queue)/4.0+settings.temperature_compensation;
    return reading;
  }

  // returns false if thermocouple is not connected or has failed open
  uint8_t IsConnected()
  {
    return !(reading&0x04);
  }
  
  // get the latest known temperature
  float Read()
  {
    return temperature;
  }
  
};

#endif
