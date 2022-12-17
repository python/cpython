from test import support
from test.support import import_helper, warnings_helper
import warnings
support.requires('audio')

from test.support import findfile

with warnings.catch_warnings():
    warnings.simplefilter("ignore", DeprecationWarning)
    ossaudiodev = import_helper.import_module('ossaudiodev')
audioop = warnings_helper.import_deprecated('audioop')
sunau = warnings_helper.import_deprecated('sunau')

import errno
import sys
import time
import unittest

# Arggh, AFMT_S16_NE not defined on all platforms -- seems to be a
# fairly recent addition to OSS.
try:
    from ossaudiodev import AFMT_S16_NE
except ImportError:
    if sys.byteorder == "little":
        AFMT_S16_NE = ossaudiodev.AFMT_S16_LE
    else:
        AFMT_S16_NE = ossaudiodev.AFMT_S16_BE


def read_sound_file(path):
    with open(path, 'rb') as fp:
        au = sunau.open(fp)
        rate = au.getframerate()
        nchannels = au.getnchannels()
        encoding = au._encoding
        fp.seek(0)
        data = fp.read()

    if encoding != sunau.AUDIO_FILE_ENCODING_MULAW_8:
        raise RuntimeError("Expect .au file with 8-bit mu-law samples")

    # Convert the data to 16-bit signed.
    data = audioop.ulaw2lin(data, 2)
    return (data, rate, 16, nchannels)

class OSSAudioDevTests(unittest.TestCase):

    def play_sound_file(self, data, rate, ssize, nchannels):
        try:
            dsp = ossaudiodev.open('w')
        except OSError as msg:
            if msg.args[0] in (errno.EACCES, errno.ENOENT,
                               errno.ENODEV, errno.EBUSY):
                raise unittest.SkipTest(msg)
            raise

        # at least check that these methods can be invoked
        dsp.bufsize()
        dsp.obufcount()
        dsp.obuffree()
        dsp.getptr()
        dsp.fileno()

        # Make sure the read-only attributes work.
        self.assertFalse(dsp.closed)
        self.assertEqual(dsp.name, "/dev/dsp")
        self.assertEqual(dsp.mode, "w", "bad dsp.mode: %r" % dsp.mode)

        # And make sure they're really read-only.
        for attr in ('closed', 'name', 'mode'):
            try:
                setattr(dsp, attr, 42)
            except (TypeError, AttributeError):
                pass
            else:
                self.fail("dsp.%s not read-only" % attr)

        # Compute expected running time of sound sample (in seconds).
        expected_time = float(len(data)) / (ssize/8) / nchannels / rate

        # set parameters based on .au file headers
        dsp.setparameters(AFMT_S16_NE, nchannels, rate)
        self.assertTrue(abs(expected_time - 3.51) < 1e-2, expected_time)
        t1 = time.monotonic()
        dsp.write(data)
        dsp.close()
        t2 = time.monotonic()
        elapsed_time = t2 - t1

        percent_diff = (abs(elapsed_time - expected_time) / expected_time) * 100
        self.assertTrue(percent_diff <= 10.0,
                        "elapsed time (%s) > 10%% off of expected time (%s)" %
                        (elapsed_time, expected_time))

    def set_parameters(self, dsp):
        # Two configurations for testing:
        #   config1 (8-bit, mono, 8 kHz) should work on even the most
        #      ancient and crufty sound card, but maybe not on special-
        #      purpose high-end hardware
        #   config2 (16-bit, stereo, 44.1kHz) should work on all but the
        #      most ancient and crufty hardware
        config1 = (ossaudiodev.AFMT_U8, 1, 8000)
        config2 = (AFMT_S16_NE, 2, 44100)

        for config in [config1, config2]:
            (fmt, channels, rate) = config
            if (dsp.setfmt(fmt) == fmt and
                dsp.channels(channels) == channels and
                dsp.speed(rate) == rate):
                break
        else:
            raise RuntimeError("unable to set audio sampling parameters: "
                               "you must have really weird audio hardware")

        # setparameters() should be able to set this configuration in
        # either strict or non-strict mode.
        result = dsp.setparameters(fmt, channels, rate, False)
        self.assertEqual(result, (fmt, channels, rate),
                         "setparameters%r: returned %r" % (config, result))

        result = dsp.setparameters(fmt, channels, rate, True)
        self.assertEqual(result, (fmt, channels, rate),
                         "setparameters%r: returned %r" % (config, result))

    def set_bad_parameters(self, dsp):
        # Now try some configurations that are presumably bogus: eg. 300
        # channels currently exceeds even Hollywood's ambitions, and
        # negative sampling rate is utter nonsense.  setparameters() should
        # accept these in non-strict mode, returning something other than
        # was requested, but should barf in strict mode.
        fmt = AFMT_S16_NE
        rate = 44100
        channels = 2
        for config in [(fmt, 300, rate),       # ridiculous nchannels
                       (fmt, -5, rate),        # impossible nchannels
                       (fmt, channels, -50),   # impossible rate
                      ]:
            (fmt, channels, rate) = config
            result = dsp.setparameters(fmt, channels, rate, False)
            self.assertNotEqual(result, config,
                             "unexpectedly got requested configuration")

            try:
                result = dsp.setparameters(fmt, channels, rate, True)
            except ossaudiodev.OSSAudioError as err:
                pass
            else:
                self.fail("expected OSSAudioError")

    def test_playback(self):
        sound_info = read_sound_file(findfile('audiotest.au'))
        self.play_sound_file(*sound_info)

    def test_set_parameters(self):
        dsp = ossaudiodev.open("w")
        try:
            self.set_parameters(dsp)

            # Disabled because it fails under Linux 2.6 with ALSA's OSS
            # emulation layer.
            #self.set_bad_parameters(dsp)
        finally:
            dsp.close()
            self.assertTrue(dsp.closed)

    def test_mixer_methods(self):
        # Issue #8139: ossaudiodev didn't initialize its types properly,
        # therefore some methods were unavailable.
        with ossaudiodev.openmixer() as mixer:
            self.assertGreaterEqual(mixer.fileno(), 0)

    def test_with(self):
        with ossaudiodev.open('w') as dsp:
            pass
        self.assertTrue(dsp.closed)

    def test_on_closed(self):
        dsp = ossaudiodev.open('w')
        dsp.close()
        self.assertRaises(ValueError, dsp.fileno)
        self.assertRaises(ValueError, dsp.read, 1)
        self.assertRaises(ValueError, dsp.write, b'x')
        self.assertRaises(ValueError, dsp.writeall, b'x')
        self.assertRaises(ValueError, dsp.bufsize)
        self.assertRaises(ValueError, dsp.obufcount)
        self.assertRaises(ValueError, dsp.obufcount)
        self.assertRaises(ValueError, dsp.obuffree)
        self.assertRaises(ValueError, dsp.getptr)

        mixer = ossaudiodev.openmixer()
        mixer.close()
        self.assertRaises(ValueError, mixer.fileno)

def setUpModule():
    try:
        dsp = ossaudiodev.open('w')
    except (ossaudiodev.error, OSError) as msg:
        if msg.args[0] in (errno.EACCES, errno.ENOENT,
                           errno.ENODEV, errno.EBUSY):
            raise unittest.SkipTest(msg)
        raise
    dsp.close()

if __name__ == "__main__":
    unittest.main()
