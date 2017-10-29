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
#ifndef __oven_hpp__
#define __oven_hpp__

#include <avr/io.h>
#include "temperaturesensor.hpp"
#include "servo.hpp"
#include "settings.hpp"

extern TemperatureSensor sensor;
extern Servo doorservo;

#define CONVECTION() (PORTD|=_BV(PD7))
#define NO_CONVECTION() (PORTD&=~_BV(PD7))
#define HEAT() (PORTD|=_BV(PD6))
#define NO_HEAT() (PORTD&=~_BV(PD6))
#define COOL() (PORTD|=_BV(PD5))
#define NO_COOL() (PORTD&=~_BV(PD5))

class Oven
{
protected:
  volatile uint8_t pwm; // 0..127
  volatile uint8_t pwmcount;
  volatile uint8_t edge;
  bool heater_on;
public:

  Oven()
  {
    pwm=0;
    edge=0;
    pwmcount=0;
    heater_on=false;
  }

  void Reset()
  {
    pwm=0;
    edge=0;
    pwmcount=0;
    NO_HEAT();
    NO_COOL();
    NO_CONVECTION();
    doorservo.SetPosition(settings.door_closed_position);
  }

  void HeaterOn() { HEAT(); }
  void HeaterOff() { NO_HEAT(); }
  void CoolerOn() { COOL(); doorservo.SetPosition(settings.door_open_position); }
  void CoolerOff() { NO_COOL(); doorservo.SetPosition(settings.door_closed_position); }
  void ConvectionOn() { CONVECTION(); }
  void ConvectionOff() { NO_CONVECTION(); }

  void SetPWM(uint8_t p)
  {
    pwm=p; 
  }

  float Temperature()
  {
    return sensor.Read();
  }
  
  bool IsFaulty()
  {
    return !sensor.IsConnected();
  }

  // this does pwm
  virtual void Run()
  {
    if (pwmcount<=edge && edge!=0)
      HeaterOn();
    else
      HeaterOff();
    pwmcount++;
    if (pwmcount>127) {
      edge=pwm;
      pwmcount=0;
    }
  }

};

#endif
