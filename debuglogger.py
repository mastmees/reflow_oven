""" The MIT License (MIT)
 
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
"""

# on OSX, the dependencies you need are py-serial, py-matplotlib, py-numpy

import serial
from io import StringIO
import matplotlib.pyplot as plt
import numpy as np
import sys
import time
import tty
import termios
import select

#derived from 
# https://stackoverflow.com/questions/21791621/python-taking-input-from-sys-stdin-non-blocking
def getch():
  old_settings = termios.tcgetattr(sys.stdin)
  ch=-1
  try:
   old_settings = termios.tcgetattr(sys.stdin)
   new_settings = termios.tcgetattr(sys.stdin)
   new_settings[3] = new_settings[3] & ~(termios.ECHO | termios.ICANON)
   new_settings[6][termios.VMIN]=0
   new_settings[6][termios.VTIME]=0
   termios.tcsetattr(sys.stdin,termios.TCSADRAIN,new_settings)
   if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
     ch = sys.stdin.read(1)
  finally:
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
  return ch

# adjust the names as needed. The FDTI device naming below is from OSX built-in driver
log=open("debug.log","a")
ser=serial.Serial("/dev/tty.usbserial-A50285BI",38400)

# on OSX the plot window does not properly close. when the chart window opens
# you have to click on the red close button on window title for debug script to
# continue but the plot window will remain open. when a new plot is made, then it
# will replace the older one. The window will close after you break the debug
# logging script

#header row has names and formats
# "time#i4,output#i4,value#f4,filteredvalue#f4"

def chart(csvtext):
  header=csvtext.partition("\n")[0]
  fields=header.split(",")
  colors=['k-','b-','r-','g-','c-','m-']
  names=[]
  formats=[]
  for f in fields:
    ff=f.split("#")
    names.append(ff[0])
    formats.append(ff[1])
  d=StringIO(unicode(csvtext))
  data=np.loadtxt(d, delimiter=',',
    dtype={'names':names,
        'formats':formats},skiprows=1,unpack=False)

  plt.close('all')
  times=[]
  for l in data:
    times.append(l[0])
  for f in range(1,len(names)):
    rows=[]
    for l in data:
      rows.append(l[f])
    plt.plot(times,rows,colors[f],label=names[f])
  plt.legend(loc="upper left")
  plt.xlabel("Time (seconds)")
  plt.grid()
  plt.show()
  
data=""
collecting=0
rows=0

def collect():
  l=""
  while True:
    if ser.in_waiting:
      c=ser.read()
      l=l+c
      if c=='\n':
        return l
    else:
      time.sleep(0.1)
    c=getch()
    if c!=-1:
      ser.write(c)
      
while 1:
  l=collect().rstrip();
  print l
  log.write(l+'\n')
  log.flush()
  if l=="Starting":
    data=""
    collecting=1
    rows=0
  elif l=="Stopping":
    if collecting==1 and rows>2:
      chart(data)
    data=""
    collecting=0
    rows=0
  elif collecting==1 and len(l) and not l.startswith("#"):
    data=data+l+"\n"
    rows=rows+1
