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
#ifndef __pid_hpp__
#define __pid_hpp__

#include <stdint.h>
#include <stdio.h>
#include "serial.hpp"

class PID
{
private:
  int8_t saturation;
  int16_t output;
  float Kp,Ki,Kd;  // coefficents
  float pe;        // previous error
  float integral;  // accumulated integral
  float Sp;        // setpoint value

protected:
  int16_t omin,omax; // output value range
    
public:

  PID() : saturation(0),output(0.0),
          pe(0.0),integral(0.0),Sp(0.0),omin(-255),omax(255)
  {
  }
  
  // initialize controller with coefficents and
  // semi-useful default input and output ranges
  PID(float kp,float ki,float kd)
  {
    PID();
    Kp=kp;
    Ki=ki;
    Kd=kd;
  }

  // set output value limits
  void SetOutputLimits(int16_t min,int16_t max)
  {
    omin=min;
    omax=max;
  }
  
  // get/set coefficents
  void SetCoefficents(float kp,float ki,float kd)
  {
    Kp=kp;
    Ki=ki;
    Kd=kd;
  }
  
  void GetCoefficents(float& kp,float& ki,float& kd)
  {
    kp=Kp;
    ki=Ki;
    kd=Kd;
  }

  // get a value of currently accumulated integral
  // (for curiosity and debugging)
  float GetIntegral()
  {
    return integral;
  }
  
  // adjust setpoint
  void SetSetPoint(float sp)
  {
    Sp=sp;
  }

  // reset error and integral, this may be useful when setpoint
  // is changed, most to discard the integral so that the output
  // would go into new direction faster
  void Reset()
  {
    integral=0;
    pe=0;
  }
  
  // get setpoint value
  float GetSetPoint(void)
  {
    return Sp;
  }

  // get latest calculated output value
  int16_t GetOutput(void)
  {
    return output;
  }
    
  // process next process value sample
  // returns calculated controller output
  int16_t ProcessInput(float value)
  {
    float e=Sp-value;
    if (saturation*e<=0)
      integral=integral+Ki*e;
    if (integral>=omin and integral<=omax)
      saturation=0;
    else {
      if (integral<omin) {
        integral=omin;
        saturation=-1;
      }
      else {
        integral=omax;
        saturation=1;
      }
    }
    float derivative=e-pe;
    pe=e;
    output=Kp*e+integral+Kd*derivative;
    if (output>omax)
      output=omax;
    if (output<omin)
      output=omin;
    return output;
  }

    
};

#endif
