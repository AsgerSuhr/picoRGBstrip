# Example of using writing a parallel byte from data
# for a more wrapped-up examples, see https://github.com/raspberrypi/pico-micropython-examples/blob/master/pio/pio_pwm.py
import rp2
from machine import Pin
from rp2 import PIO, StateMachine, asm_pio
from time import sleep
import math

# seems like there's something off with micropythons interpretation of 32 bits that I can't figure out.
# might so halfing the values like the brightness and the color bit seems to give a better result

# actually according to the docs for APA120 the brightness range is from 0 - 31, but seems to be 0 - 15 here in micropython.
# works in C tho'
BRIGHTNESS = 15

@asm_pio(sideset_init=PIO.OUT_HIGH, out_init=(rp2.PIO.OUT_HIGH,), out_shiftdir=rp2.PIO.SHIFT_LEFT, fifo_join=PIO.JOIN_TX, autopull=True, pull_thresh=32 )
def paral_prog():    
    out(pins, 1)  .side(1)
    nop()         .side(0)
    
def put_rgb888(sm, brightness, r, g, b):
    sm.put(0x7 << 29 |
           (brightness & 0x1f) << 24 |
           int(b) << 16 |
           int(g) << 8 |
           int(r) << 0)
    return f'{r} {g} {b} == {0x7 << 29 |(brightness & 0x1f) << 24 | int(b) << 16 | int(g) << 8 | int(r) << 0}'

def set_bit(value, bit):
    return value | (1<<bit)

def clear_bit(value, bit):
    return value & ~(1<<bit)


if __name__ == '__main__':    
    paral_sm = StateMachine(0, paral_prog, freq=1 * 1000 * 1000, sideset_base=Pin(2), out_base=Pin(3))
    #parallel data out pins 0-7, Data Ready out pin 16, 
    paral_sm.active(1)
    
    # again the instead of going to 255 in the color range (ex. [255, 255, 255]) I'm capping the value at 127 instead
    TABLE_SIZE = (1 << 8)
    wave_table = [0 for x in range(TABLE_SIZE)]
    for i in range(TABLE_SIZE):
        wave_table[i] = int(pow(math.sin(i * math.pi / TABLE_SIZE), 5)*127)
    print(wave_table)
    
    t = 0
    while True:
        paral_sm.put(0)
        for i in range(30):
            
            put_rgb888(paral_sm,
                       BRIGHTNESS,
                       wave_table[(i + t) % TABLE_SIZE],
                       wave_table[(2 * i + 3 * 2) % TABLE_SIZE],
                       wave_table[(3 * i + 4 * t) % TABLE_SIZE])
            
            #put_rgb888(paral_sm, 220, 0, 0)
        paral_sm.put(1)
        #sleep(0.0001)
        t+= 1
        #if t == 4:
            #break
