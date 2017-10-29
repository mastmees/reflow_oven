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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <string.h>

#include "button.hpp"
#include "pid.hpp"
#include "serial.hpp"
#include "process.hpp"
#include "servo.hpp"
#include "settings.hpp"
#include "oven.hpp"

uint16_t tick_counter;
int32_t second_counter;

TemperatureSensor sensor;
Button startbutton;
Button profilebutton;
Serial serial;
Process process;
Servo doorservo;
Settings settings;
Oven oven;

extern PID pidcontroller;

Settings EEMEM ee_settings {
 0.0, // thermocouple reading compensation (degc)
 16.0,0.05,2.1, // PID controller parameters
 240, // servo position for closed door
 124 // servo position for open door
};

void Help()
{
  serial.print(
    "\n#Commands"
    "\n# ? this help"
    "\n# g go to temperature"
    "\n# s settings"
    "\n# t current temperature"
    "\n# o open door"
    "\n# c close door"
    "\n# T set temperature comp"
    "\n# P set PID P"
    "\n# I set PID I"
    "\n# D set PID D"
    "\n# O set door open position"
    "\n# C set door closed position"
    "\n"
  );
}

void ReadSettings()
{
  eeprom_read_block(&settings,&ee_settings,sizeof(settings));
  serial.print("\n#Settings\n");
  serial.print("# temperature comp: ",settings.temperature_compensation);
  serial.print("# P: ",settings.P);
  serial.print("# I: ",settings.I);
  serial.print("# D: ",settings.D);
  serial.print("# door open position: ",(int32_t)settings.door_open_position);
  serial.print("# door closed position: ",(int32_t)settings.door_closed_position);
  serial.print("\n");
}

void WriteSettings()
{
  eeprom_write_block(&settings,&ee_settings,sizeof(settings));
}

bool Input(char *buf,uint8_t buflen)
{
  uint8_t c;
  if (!buf || buflen<1)
    return false;
  while (buflen) {
    wdt_reset();
    WDTCSR|=0x40;
    if (serial.rxready()) {
      c=serial.receive();
      if (c=='\n') {
        *buf='\0';
        return true;              
      }
      if (c=='\x1b') {
        return false;
      }
      *buf++=c;
      buflen--;
    }
  }
  return false;
}

bool InputFloat(float& f)
{
  char buf[16],*s;
  bool neg=false;
  int32_t v=0;
  uint8_t dp=0;
  f=0.0;
  if (Input(buf,sizeof buf)) {
    s=buf;
    while (*s) {
      switch (*s) {
        case '\0':
          break;
        case '-':
          neg=true;
          break;
        case '.':
          dp=1;
          break;
        default:
          if (*s>='0' && *s<='9') {
            v=v*10+(*s-'0');
            if (dp)
              dp++;
          }
          else
            return false;
          break;
      }
      s++;
    }
    int32_t d=1;
    while (dp>1) {
      dp--;
      d*=10;
    }
    if (neg)
      v=0-v;
    f=float(v)/(float)d;
    return true;
  }
  return false;
}

void ModifySetting(const char * prompt,float& f)
{
float v;
  serial.print("\n");
  serial.print(prompt);
  serial.print("\n");
  if (InputFloat(v)) {
    f=v;
    WriteSettings();
    ReadSettings();
  }
}

void ModifySetting(const char * prompt,uint8_t& u)
{
float v;
  serial.print("\n");
  serial.print(prompt);
  serial.print("\n");
  if (InputFloat(v)) {
    u=(uint8_t)v;
    WriteSettings();
    ReadSettings();
  }
}


void ProcessSerialInput()
{
static uint8_t busy;
float f;
int16_t pidoutput;
int32_t oc;
  if (busy)
    return;
  busy++;
  if (serial.rxready()) {
    switch (serial.receive()) {
      case 'g':
        pidcontroller.SetOutputLimits(-127,127);
        pidcontroller.SetCoefficents(settings.P,settings.I,settings.D);
        pidcontroller.Reset();
        serial.print("\n#Enter setpoint:\n");
        if (InputFloat(f)) {
          oven.Reset();
          pidcontroller.SetSetPoint(f);
          second_counter=0;
          oc=-1;
          serial.print("\nStarting\n");
          serial.print("time#i4,sepoint#f4,temperature#f4,output#i4,integrator#f4\n");
          while (!serial.rxready()) {
            if (oc!=second_counter) {
              oc=second_counter;
              f=oven.Temperature();
              pidoutput=pidcontroller.ProcessInput(f);
              oven.SetPWM(pidoutput>=0?pidoutput:0);
              serial.print(second_counter);
              serial.send(',');
              serial.print(pidcontroller.GetSetPoint());
              serial.send(',');
              serial.print(f);
              serial.send(',');
              serial.print((int32_t)pidoutput);
              serial.send(',');
              serial.print(pidcontroller.GetIntegral());
              serial.send('\n');
            }
            wdt_reset();
            WDTCSR|=0x40;
          }
          oven.Reset();
          serial.receive();
          serial.print("Stopping\n");
        }
        break;
      case 's':
        ReadSettings();
        break;
      case '?':
        Help();
        break;
      case 'o':
        doorservo.SetPosition(settings.door_open_position);
        serial.print("\n#Door open\n");
        break;
      case 'c':
        doorservo.SetPosition(settings.door_closed_position);
        serial.print("\n#Door closed\n");
        break;
      case 't':
        serial.print("\n#Current temperature: ");
        serial.print(sensor.Read());
        serial.print("\n");
        break;
      case 'T':
        ModifySetting("#Enter temperature compensation:",settings.temperature_compensation);
        break;
      case 'P':
        ModifySetting("#Enter PID P value:",settings.P);
        break;
      case 'I':
        ModifySetting("#Enter PID I value:",settings.I);
        break;
      case 'D':
        ModifySetting("#Enter PID D value:",settings.D);
        break;
      case 'O':
        ModifySetting("#Enter door open position:",settings.door_open_position);
        break;
      case 'C':
        ModifySetting("#Enter door closed position:",settings.door_closed_position);
        break;
    }
  }
  busy--;
}

// screen /dev/tty.usbserial-A50285BI 38400,n,8,1

ISR(TIMER0_OVF_vect)
{
static uint8_t sensorcounter,servocounter,ovencounter;
  // reset timer for next interrupt
  TCNT0=2;
  servocounter++;
  if (servocounter>4) {
    doorservo.Pulse();
    servocounter=0;
  }

  sensorcounter++;
  if (sensorcounter>59) {
    sensor.RawRead();
    sensorcounter=0;
  }

  ovencounter++;
  if (ovencounter>0) {
    oven.Run();
    ovencounter=0;
  }
  
  tick_counter++;
  if (tick_counter>=246) { // 246 for 1 second
    tick_counter=0;
    second_counter++;
  }

  profilebutton.Update(PINB&_BV(PB1));
  startbutton.Update(PIND&_BV(PD2));
}

ISR(WDT_vect)
{
}

/*
I/O configuration
-----------------
I/O pin                               direction    DDR  PORT
PC0 unused                            output       1    1
PC1 unused                            output       1    1
PC2 unused                            output       1    1
PC3 unused                            output       1    1
PC4 unused                            output       1    1
PC5 unused                            output       1    1

PD0 RxD                               input        0    0
PD1 TxD                               input        0    0
PD2 start/stop button                 input, p-up  0    1
PD3 servo pulse                       output       1    0
PD4 profile leds                      output       1    1
PD5 cooling drive                     output       1    0
PD6 heating drive                     output       1    0
PD7 convection drive                  output       1    0

PB0 MAX6675 CS                        output       1    1
PB1 profile select                    input        0    1
PB2 MAX6675 DO                        input,p-up   0    1
PB3 SPI MOSI                          output       1    1
PB4 SPI MISO                          input, p-up  0    1
PB5 SPI SCK                           output       1    0
*/
int main(void)
{
  MCUSR=0;
  MCUCR=0;
  // I/O directions
  DDRC=0x3f;
  DDRD=0xf8;
  DDRB=0x29;
  // initial state
  PORTC=0x3f;
  PORTD=0x14;
  PORTB=0x1f;
  //while ((PINC&4)==0) // wait for INT0 input to go high
  //  ;
  //EICRA=2; // falling edge on INT0 interrupts
  //enable_button(); // enable INT0 interrupts
  //
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  // configure watchdog to interrupt&reset, 4 sec timeout
  WDTCSR|=0x18;
  WDTCSR=0xe8;
  // configure timer0 for periodic interrupts
  TCCR0B=4; // timer0 clock prescaler to 256
  TIMSK0=1; // enable overflow interrupts
  serial.enable();
  ReadSettings();
  sei();
  while (1) {
    sleep_cpu(); // any interrupt (which can be only timer or watchdog) wakes up
    wdt_reset();
    WDTCSR|=0x40;
    process.Run();
    ProcessSerialInput();
  }
}
