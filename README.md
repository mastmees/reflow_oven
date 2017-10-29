# reflow_oven
Firmware for a reflow soldering oven

A full project page documenting my DIY reflow soldering oven build is
at http://www.nomad.ee/micros/oven/

The current version of firmware has two soldering profiles, for leaded
solder and lead-free. My oven has only active heating, no convection or
over cooling fan. There is a servo control for pushing the door open at
the end. Although the temperature is contoller by PID controller, large
part of getting the profiles as you want is to tweak the soldering profile
steps for your oven.

The code also includes a python script that you can use as serial terminal
for your oven, and it plots charts for the reflow process.

