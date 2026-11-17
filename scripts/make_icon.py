#!/usr/bin/env python3
"""Render the app's battery + ASCII-trace icon; standard library only."""
from pathlib import Path
import struct
import zlib
size=256
pixels=bytearray(bytes((7,16,19))*size*size)
def rect(x,y,w,h,color):
    for row in range(y,y+h):
        for col in range(x,x+w):
            offset=(row*size+col)*3
            pixels[offset:offset+3]=bytes(color)
cyan=(104,196,206)
green=(102,220,138)
rect(28,61,184,128,cyan)
rect(36,69,168,112,(7,16,19))
rect(212,99,16,52,cyan)
for col,height in enumerate([2,3,5,7,4,2,3,6,5,3]):
    for row in range(height):
        rect(47+col*15,158-row*12,10,8,green)
def chunk(kind,data):
    return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
raw=b''.join(b'\0'+pixels[y*size*3:(y+1)*size*3] for y in range(size))
output=Path(__file__).resolve().parents[1]/'applaunch/share/images/zero-watt.png'
output.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',size,size,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
