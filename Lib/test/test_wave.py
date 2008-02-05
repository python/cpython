from test.test_support import TESTFN, run_unittest
import os
import wave
import unittest

nchannels = 2
sampwidth = 2
framerate = 8000
nframes = 100

class TestWave(unittest.TestCase):

    def setUp(self):
        self.f = None

    def tearDown(self):
        if self.f is not None:
            self.f.close()
        try:
            os.remove(TESTFN)
        except OSError:
            pass

    def test_it(self):
        self.f = wave.open(TESTFN, 'wb')
        self.f.setnchannels(nchannels)
        self.f.setsampwidth(sampwidth)
        self.f.setframerate(framerate)
        self.f.setnframes(nframes)
        output = '\0' * nframes * nchannels * sampwidth
        self.f.writeframes(output)
        self.f.close()

        self.f = wave.open(TESTFN, 'rb')
        self.assertEqual(nchannels, self.f.getnchannels())
        self.assertEqual(sampwidth, self.f.getsampwidth())
        self.assertEqual(framerate, self.f.getframerate())
        self.assertEqual(nframes, self.f.getnframes())
        self.assertEqual(self.f.readframes(nframes), output)

def test_main():
    run_unittest(TestWave)

if __name__ == '__main__':
    test_main()
