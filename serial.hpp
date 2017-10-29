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
#ifndef __serial_hpp__
#define __serial_hpp__
#include <avr/io.h>

#define BAUDRATE 38400L
#define UBRR (F_CPU/(16*BAUDRATE)-1)

class Serial
{
public:
  
  void enable()
  {
    if (!(DDRD&2)) {
      DDRD|=0x02;
      PORTD|=0x01;
      UBRR0H=(unsigned char)(UBRR>>8);
      UBRR0L=(unsigned char)UBRR;
      UCSR0B=(1<<RXEN0)|(1<<TXEN0);
      UCSR0C=(1<<USBS0)|(3<<UCSZ00);
    }
  }
  
  void disable()
  {
    if (DDRD&2) {
      UCSR0B=0;
      DDRD&=0xfc;
      PORTD&=0xfc;
    }
  }

  bool rxready()
  {
    return (UCSR0A&(1<<RXC0));
  }  
  
  uint8_t receive()
  {
    while (!rxready()); // this blocks the caller if no data received
    return UDR0;
  }

  void send(uint8_t c)  
  {
    while (!(UCSR0A&(1<<UDRE0)));
    UDR0 = c;
  }
  
  void print(const char *s)
  {
    while (s && *s)
      send(*s++);
  }
  
  void print(int32_t n)
  {
    if (n<0) {
      send('-');
      n=0-n;
    }
    if (n>9)
      print(n/10);
    send((n%10)+'0');
  }

  void print(float v)
  {
     int n;
     print((int32_t)v);
     if (v<0.0)
       v=0.0-v;
     send('.');
     n=(v-(int32_t)v)*1000;
     send(n/100+'0');
     n=n%100;
     send(n/10+'0');
     n=n%10;
     send(n+'0');
  }
  
  void print(const char *s,float v)
  {
    print(s);
    print(v);
    send('\n');
  }
  
  void print(const char *s,int32_t n)
  {
    print(s);
    print(n);
    send('\n');
  }
    
};

#endif
