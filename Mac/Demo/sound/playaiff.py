from Carbon.Sound import *
from Carbon import Snd

import aifc, audioop

fn = 'f:just samples:2ndbeat.aif'
af = aifc.open(fn, 'r')
print af.getparams()
print 'nframes  =', af.getnframes()
print 'nchannels =', af.getnchannels()
print 'framerate =', af.getframerate()
nframes = min(af.getnframes(), 100000)
frames = af.readframes(nframes)
print 'len(frames) =', len(frames)
print repr(frames[:100])
frames = audioop.add(frames, '\x80'*len(frames), 1)
print repr(frames[:100])

import struct

header1 = struct.pack('llhhllbbl',
                      0,
                      af.getnchannels(),
                      af.getframerate(),0,
                      0,
                      0,
                      0xFF,
                      60,
                      nframes)
print repr(header1)
header2 = struct.pack('llhlll', 0, 0, 0, 0, 0, 0)
header3 = struct.pack('hhlll',
                      af.getsampwidth()*8,
                      0,
                      0,
                      0,
                      0)
print repr(header3)
header = header1 + header2 + header3

buffer = header + frames

chan = Snd.SndNewChannel(5,0x00C0)

Snd.SndDoCommand(chan, (bufferCmd, 0, buffer), 0)
