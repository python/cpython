from test.support import run_unittest, TESTFN
import unittest
import os

import sunau

nchannels = 2
sampwidth = 2
framerate = 8000
nframes = 100

class SunAUTest(unittest.TestCase):

    def setUp(self):
        self.f = None

    def tearDown(self):
        if self.f is not None:
            self.f.close()
        try:
            os.remove(TESTFN)
        except OSError:
            pass

    def test_lin(self):
        self.f = sunau.open(TESTFN, 'w')
        self.f.setnchannels(nchannels)
        self.f.setsampwidth(sampwidth)
        self.f.setframerate(framerate)
        self.f.setcomptype('NONE', 'not compressed')
        output = b'\xff\x00\x12\xcc' * (nframes * nchannels * sampwidth // 4)
        self.f.writeframes(output)
        self.f.close()

        self.f = sunau.open(TESTFN, 'rb')
        self.assertEqual(nchannels, self.f.getnchannels())
        self.assertEqual(sampwidth, self.f.getsampwidth())
        self.assertEqual(framerate, self.f.getframerate())
        self.assertEqual(nframes, self.f.getnframes())
        self.assertEqual('NONE', self.f.getcomptype())
        self.assertEqual(self.f.readframes(nframes), output)
        self.f.close()

    def test_ulaw(self):
        self.f = sunau.open(TESTFN, 'w')
        self.f.setnchannels(nchannels)
        self.f.setsampwidth(sampwidth)
        self.f.setframerate(framerate)
        self.f.setcomptype('ULAW', '')
        # u-law compression is lossy, therefore we can't expect non-zero data
        # to come back unchanged.
        output = b'\0' * nframes * nchannels * sampwidth
        self.f.writeframes(output)
        self.f.close()

        self.f = sunau.open(TESTFN, 'rb')
        self.assertEqual(nchannels, self.f.getnchannels())
        self.assertEqual(sampwidth, self.f.getsampwidth())
        self.assertEqual(framerate, self.f.getframerate())
        self.assertEqual(nframes, self.f.getnframes())
        self.assertEqual('ULAW', self.f.getcomptype())
        self.assertEqual(self.f.readframes(nframes), output)
        self.f.close()


def test_main():
    run_unittest(SunAUTest)

if __name__ == "__main__":
    unittest.main()
