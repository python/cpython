from test_support import verbose, findfile, TestFailed, TestSkipped

import errno
import fcntl
import linuxaudiodev
import os
import sys
import select
import sunaudio
import time
import audioop

SND_FORMAT_MULAW_8 = 1

def play_sound_file(path):
    fp = open(path, 'r')
    size, enc, rate, nchannels, extra = sunaudio.gethdr(fp)
    data = fp.read()
    fp.close()

    if enc != SND_FORMAT_MULAW_8:
        print "Expect .au file with 8-bit mu-law samples"
        return

    try:
        a = linuxaudiodev.open('w')
    except linuxaudiodev.error, msg:
        if msg[0] in (errno.EACCES, errno.ENODEV):
            raise TestSkipped, msg
        raise TestFailed, msg

    # convert the data to 16-bit signed
    data = audioop.ulaw2lin(data, 2)

    # set the data format
    if sys.byteorder == 'little':
        fmt = linuxaudiodev.AFMT_S16_LE
    else:
        fmt = linuxaudiodev.AFMT_S16_BE

    # at least check that these methods can be invoked
    a.bufsize()
    a.obufcount()
    a.obuffree()
    a.getptr()
    a.fileno()

    # set parameters based on .au file headers
    a.setparameters(rate, 16, nchannels, fmt)
    a.write(data)
    a.flush()
    a.close()

def test_errors():
    a = linuxaudiodev.open("w")
    size = 8
    fmt = linuxaudiodev.AFMT_U8
    rate = 8000
    nchannels = 1
    try:
        a.setparameters(-1, size, nchannels, fmt)
    except ValueError, msg:
        print msg
    try:
        a.setparameters(rate, -2, nchannels, fmt)
    except ValueError, msg:
        print msg
    try:
        a.setparameters(rate, size, 3, fmt)
    except ValueError, msg:
        print msg
    try:
        a.setparameters(rate, size, nchannels, 177)
    except ValueError, msg:
        print msg
    try:
        a.setparameters(rate, size, nchannels, linuxaudiodev.AFMT_U16_LE)
    except ValueError, msg:
        print msg
    try:
        a.setparameters(rate, 16, nchannels, fmt)
    except ValueError, msg:
        print msg

def test():
    play_sound_file(findfile('audiotest.au'))
    test_errors()

test()
