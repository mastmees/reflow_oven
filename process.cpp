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
#include "process.hpp"
#include "settings.hpp"

extern Button startbutton;

PID pidcontroller(10.0,0.0,0.0);

extern Oven oven;

#define SHOWPROFILE1() (PORTD|=_BV(PD4))
#define SHOWPROFILE0() (PORTD&=(~_BV(PD4)))
#define FAULT_BLINK() (PORTD^=_BV(PD4))

// assume starting at 25degC
// normal heating rate 2 degC/sec
// normal cooling rate 3 degC/sec

struct ProfileStep leadedsteps[] = {
  { ProfileStep::DOOR_CLOSE, 0},
  { 100, 60 }, // heat up to 100, this will lag and overshoot
  { 120, 30 }, // ride the overshoot to up to 150
  { 150, 40 }, // get to preheat temperature, still overshooting here
  { 150, 60 }, // stay at preheat
  { 185, 30 }, // ramp to reflow
  { 210, 20 }, // fast towards peak reflow, this will also overshoot
  { 228, 10 }, // reset controller then take last step
  { 228, 3 }, // reset controller then take last step
  { ProfileStep::DOOR_OPEN, 0},
  { 60, 60  }, // rapid cooldown
  { ProfileStep::PROCESS_DONE, 0  }  // done
};

struct ProfileStep leadfreesteps[] = {
  { ProfileStep::DOOR_CLOSE,0 },
  { 100, 60 },
  { 125, 30 },
  { 160, 20 },
  { 180, 30 },
  { 200,30 }, // preheat, ramp to 200 in 90 seconds
  { 200,5 }, // hold for 5 seconds
  { 210,25 }, // ramp up to reflow temp at nominal rate
  { 235,10 },
  { 250,10 }, // ramp up to peak reflow at faster rate
  { 250,4  },  // stay at peak for 4 seconds
  { ProfileStep::DOOR_OPEN,0 },
  { 60,60  },  // cool down to 60deg
  { ProfileStep::PROCESS_DONE, 0   }    // done
};

struct Profile leadedprofile = {
  &leadedsteps[0],
  160,
  200
};

struct Profile leadfreeprofile = {
  &leadfreesteps[0],
  190,
  215
};

void Process::SetProfile(Profile *p)
{
  profile=p;
  step=&p->steps[0];
  while (1) {
    if (step->temp==ProfileStep::PROCESS_DONE) {
      state=STOPPING;
      serial.print("#no steps in process?\n");
      return;
    }
    if (step->temp==ProfileStep::DOOR_OPEN) {
      oven.CoolerOn();
      serial.print("#opening door\n");
      step++;
      continue;
    }
    if (step->temp==ProfileStep::DOOR_CLOSE) {
      oven.CoolerOff();
      serial.print("#closing door\n");
      step++;
      continue;
    }
    if (step->temp>0)
      break;
    step++; // safeguard for special steps not handled here
  }
  targettemp=step->temp;
  setpoint=oven.Temperature();
  if (step->seconds==0)
    setpointstep=targettemp-setpoint;
  else
    setpointstep=(targettemp-setpoint)/step->seconds;
  pidcontroller.SetSetPoint(setpoint);
  pidcontroller.Reset();
  runningtime=0;
}

void Process::ProcessTick()
{
  float v=oven.Temperature();
  if (step->seconds>runningtime) { // minimum time not expired yet
    setpoint+=setpointstep;
    pidcontroller.SetSetPoint(setpoint);
  }
  else {
    pidcontroller.SetSetPoint(targettemp);
    setpoint=targettemp;
    if (setpointstep>=0.0) { // ramping upwards
      if (v>=targettemp) {
        while (1) {
          step++;
          if (step->temp==ProfileStep::PROCESS_DONE) {
            state=STOPPING;
            serial.print("#last step reached\n");
            return;
          }
          if (step->temp==ProfileStep::DOOR_OPEN) {
            oven.CoolerOn();
            serial.print("#opening door\n");
            continue;
          }
          if (step->temp==ProfileStep::DOOR_CLOSE) {
            oven.CoolerOff();
            serial.print("#closing door\n");
            continue;
          }
          if (step->temp>0)
            break;
        }
        if (step->temp==targettemp)
          setpointstep=0.0;
        else {
          targettemp=step->temp;
          if (step->seconds)
            setpointstep=(targettemp-v)/step->seconds;
          else
            setpointstep=(targettemp-v);
        }
        runningtime=0;
        pidcontroller.Reset();
      } 
    }
    else { // ramping downwards or steady
      if (v<=targettemp) {
        while (1) {
          step++;
          if (step->temp==ProfileStep::PROCESS_DONE) {
            state=STOPPING;
            serial.print("#last step reached\n");
            return;
          }
          if (step->temp==ProfileStep::DOOR_OPEN) {
            oven.CoolerOn();
            serial.print("#opening door\n");
            continue;
          }
          if (step->temp==ProfileStep::DOOR_CLOSE) {
            oven.CoolerOff();
            serial.print("#closing door\n");
            continue;
          }
          if (step->temp>0)
            break;
        }
        if (step->temp==targettemp)
          setpointstep=0.0;
        else {
          targettemp=step->temp;
          if (step->seconds)
            setpointstep=(targettemp-v)/step->seconds;
          else
            setpointstep=targettemp-v;
        }
        runningtime=0;
        pidcontroller.Reset();
      }
    }
  }
  runningtime++;
}

Process::Process()
{
  state=STOPPING;
  timestamp=(unsigned int)-1;
  pidoutput=0;
  profile=NULL;
  pwmcounter=0;
  targettemp=0.0;
  setpointstep=0.0;
  setpoint=0.0;
}

void Process::Run()
{
  float v;
  switch (state) {
    case STOPPING:
      serial.print("Stopping\n");
      oven.Reset();
      startbutton.Clear();
      state=STOPPED;
      break;
    case STOPPED:
      if (oven.IsFaulty()) {
        state=FAULT;
        break;
      }
      if (profilebutton.Pressed())
        SHOWPROFILE1();
      else
        SHOWPROFILE0();
      if (startbutton.Read())
        state=STARTING;
      break;
    case STARTING:
      pidcontroller.SetOutputLimits(-127,127);
      pidcontroller.SetCoefficents(settings.P,settings.I,settings.D);
      if (profilebutton.Pressed()) {
        serial.print("#Lead-free profile\n");
        SetProfile(&leadfreeprofile);
        SHOWPROFILE1();
      }
      else {
        serial.print("#Leaded profile\n");
        SetProfile(&leadedprofile);
        SHOWPROFILE0();
      }
      serial.print("Starting\n");
      serial.print("time#i4,target#f4,setpoint#f4,temperature#f4,pidoutput#i4\n");
      second_counter=0;
      oven.ConvectionOn();
      oven.CoolerOff();
      state=RUNNING;
      break;
    case RUNNING:
      if (oven.IsFaulty()) {
        state=FAULT;
        break;
      }
      //
      if (startbutton.Read())
        state=STOPPING;
      //
      if (timestamp!=second_counter && targettemp>=0.0) {
        v=oven.Temperature();
        pidoutput=pidcontroller.ProcessInput(v);
        if (pidoutput>=0) {
          oven.SetPWM(pidoutput);
        }
        else {
          oven.SetPWM(0);
        }
        ProcessTick();
        serial.print(second_counter);
        serial.send(',');
        serial.print(targettemp);
        serial.send(',');
        serial.print(setpoint);
        serial.send(',');
        serial.print(v);
        serial.send(',');
        serial.print((int32_t)pidoutput);
        serial.send('\n');
      }
      break;
    case FAULT:
      serial.print("#Fault\n");
      oven.Reset();
      state=BLINKING;
      break;
    case BLINKING:
      if (timestamp!=second_counter) {
        FAULT_BLINK();
      }
      if (!oven.IsFaulty())
      {
        serial.print("#Fault cleared\n");
        state=STOPPING;
      }
      break;      
  }
  timestamp=second_counter;
}
