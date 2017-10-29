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
#ifndef __process_hpp__
#define __process_hpp__

#include <avr/io.h>
#include "oven.hpp"
#include "pid.hpp"
#include "serial.hpp"
#include "button.hpp"

extern Button profilebutton;
extern Button startbutton;
extern Serial serial;
extern int32_t second_counter;

 
struct ProfileStep
{
  enum STEP { PROCESS_DONE=-1, DOOR_OPEN=-2, DOOR_CLOSE=-3 };
  int temp;    // desired temperature at the end of the step, or one of enums
  int seconds; // minimum number of seconds the step lasts
};

struct Profile {
  ProfileStep *steps; // actual profile
  int lowcritical;      // low limit of critical temperature range around liquous
  int highcritical;     // high limit of critical temperature range around liquous
};

class Process 
{
  enum PROCESS_STATE { STOPPED,STARTING,RUNNING,STOPPING,FAULT,BLINKING };
  PROCESS_STATE state;
  unsigned int timestamp;
  int16_t pidoutput;
  Profile *profile;
  ProfileStep *step;
  uint8_t pwmcounter;
  float targettemp,setpointstep,setpoint;
  int minimumtime,runningtime;
   
  void SetProfile(Profile *p);
  void ProcessTick();
  
public:
  Process();
  void Run();

};

#endif
