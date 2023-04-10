# APA102 LED strip project

Intended to be used with Home Assistant. I wanted to have a pico control the LED strips I intend
to install above my hobby desk, so that I could have fancy colored lights when I work (Might never use it, but colors are fun).

Since I got an APA102 LED strip I needed to use PIO to communicate with the device, since it doesn't have a standard interface. Some people seem to 
have gotten it work with SPI but I couldn't find a library, and it seemed unintuitive. Also there was a PIO example with the APA102 in pico-examples. 
I tried to convert it to micropython as well, before I got the Pico SDK wifi connection to work. 

Seems to work alright in micropython except for one hurdle I can't get past. When I bit shift the brightness, and the RGB values to 32 bits I seem to lose
half of my range. I'm sending something wrong to the APA102 since it breaks if my RGB values go past 127 (rounded half of 255) and if I 
go past 15 in brightness (which is around half of 31). Works in C though. 