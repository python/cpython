from test import test_support
test_support.requires('audio')

from test.test_support import verbose, findfile, TestFailed, TestSkipped

import errno
import fcntl
import ossaudiodev
import os
import sys
import select
import sunaudio
import time
import audioop

SND_FORMAT_MULAW_8 = 1

def read_sound_file(path):
    fp = open(path, 'rb')
    size, enc, rate, nchannels, extra = sunaudio.gethdr(fp)
    data = fp.read()
    fp.close()

    if enc != SND_FORMAT_MULAW_8:
        print "Expect .au file with 8-bit mu-law samples"
        return

    # Convert the data to 16-bit signed.
    data = audioop.ulaw2lin(data, 2)
    return (data, rate, 16, nchannels)


def play_sound_file(data, rate, ssize, nchannels):
    try:
        a = ossaudiodev.open('w')
    except IOError, msg:
        if msg[0] in (errno.EACCES, errno.ENODEV, errno.EBUSY):
            raise TestSkipped, msg
        raise TestFailed, msg

    # set the data format
    if sys.byteorder == 'little':
        fmt = ossaudiodev.AFMT_S16_LE
    else:
        fmt = ossaudiodev.AFMT_S16_BE

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
    a = ossaudiodev.open("w")
    size = 8
    fmt = ossaudiodev.AFMT_U8
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
        a.setparameters(rate, size, nchannels, ossaudiodev.AFMT_U16_LE)
    except ValueError, msg:
        print msg
    try:
        a.setparameters(rate, 16, nchannels, fmt)
    except ValueError, msg:
        print msg

def test():
    (data, rate, ssize, nchannels) = read_sound_file(findfile('audiotest.au'))
    play_sound_file(data, rate, ssize, nchannels)
    test_errors()

test()
