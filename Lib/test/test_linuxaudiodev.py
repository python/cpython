from test import test_support
test_support.requires('audio')

from test.test_support import findfile, run_unittest

import errno
import sys
import audioop
import unittest

linuxaudiodev = test_support.import_module('linuxaudiodev', deprecated=True)
sunaudio = test_support.import_module('sunaudio', deprecated=True)

SND_FORMAT_MULAW_8 = 1

class LinuxAudioDevTests(unittest.TestCase):

    def setUp(self):
        self.dev = linuxaudiodev.open('w')

    def tearDown(self):
        self.dev.close()

    def test_methods(self):
        # at least check that these methods can be invoked
        self.dev.bufsize()
        self.dev.obufcount()
        self.dev.obuffree()
        self.dev.getptr()
        self.dev.fileno()

    def test_play_sound_file(self):
        path = findfile("audiotest.au")
        fp = open(path, 'r')
        size, enc, rate, nchannels, extra = sunaudio.gethdr(fp)
        data = fp.read()
        fp.close()

        if enc != SND_FORMAT_MULAW_8:
            self.fail("Expect .au file with 8-bit mu-law samples")

        # convert the data to 16-bit signed
        data = audioop.ulaw2lin(data, 2)

        # set the data format
        if sys.byteorder == 'little':
            fmt = linuxaudiodev.AFMT_S16_LE
        else:
            fmt = linuxaudiodev.AFMT_S16_BE

        # set parameters based on .au file headers
        self.dev.setparameters(rate, 16, nchannels, fmt)
        self.dev.write(data)
        self.dev.flush()

    def test_errors(self):
        size = 8
        fmt = linuxaudiodev.AFMT_U8
        rate = 8000
        nchannels = 1
        try:
            self.dev.setparameters(-1, size, nchannels, fmt)
        except ValueError, err:
            self.assertEqual(err.args[0], "expected rate >= 0, not -1")
        try:
            self.dev.setparameters(rate, -2, nchannels, fmt)
        except ValueError, err:
            self.assertEqual(err.args[0], "expected sample size >= 0, not -2")
        try:
            self.dev.setparameters(rate, size, 3, fmt)
        except ValueError, err:
            self.assertEqual(err.args[0], "nchannels must be 1 or 2, not 3")
        try:
            self.dev.setparameters(rate, size, nchannels, 177)
        except ValueError, err:
            self.assertEqual(err.args[0], "unknown audio encoding: 177")
        try:
            self.dev.setparameters(rate, size, nchannels, linuxaudiodev.AFMT_U16_LE)
        except ValueError, err:
            self.assertEqual(err.args[0], "for linear unsigned 16-bit little-endian "
                             "audio, expected sample size 16, not 8")
        try:
            self.dev.setparameters(rate, 16, nchannels, fmt)
        except ValueError, err:
            self.assertEqual(err.args[0], "for linear unsigned 8-bit audio, expected "
                             "sample size 8, not 16")

def test_main():
    try:
        dsp = linuxaudiodev.open('w')
    except linuxaudiodev.error, msg:
        if msg.args[0] in (errno.EACCES, errno.ENOENT, errno.ENODEV, errno.EBUSY):
            raise unittest.SkipTest(msg)
        raise
    dsp.close()
    run_unittest(LinuxAudioDevTests)

if __name__ == '__main__':
    test_main()
