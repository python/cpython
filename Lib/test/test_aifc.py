from test.support import findfile, run_unittest
import unittest

import aifc


class AIFCTest(unittest.TestCase):

    def setUp(self):
        self.sndfilepath = findfile('Sine-1000Hz-300ms.aif')

    def test_skipunknown(self):
        #Issue 2245
        #This file contains chunk types aifc doesn't recognize.
        f = aifc.open(self.sndfilepath)
        f.close()

    def test_params(self):
        f = aifc.open(self.sndfilepath)
        self.assertEqual(f.getnchannels(), 2)
        self.assertEqual(f.getsampwidth(), 2)
        self.assertEqual(f.getframerate(), 48000)
        self.assertEqual(f.getnframes(), 14400)
        self.assertEqual(f.getcomptype(), b'NONE')
        self.assertEqual(f.getcompname(), b'not compressed')
        self.assertEqual(
            f.getparams(),
            (2, 2, 48000, 14400, b'NONE', b'not compressed'),
            )
        f.close()

    def test_read(self):
        f = aifc.open(self.sndfilepath)
        self.assertEqual(f.tell(), 0)
        self.assertEqual(f.readframes(2), b'\x00\x00\x00\x00\x0b\xd4\x0b\xd4')
        f.rewind()
        pos0 = f.tell()
        self.assertEqual(pos0, 0)
        self.assertEqual(f.readframes(2), b'\x00\x00\x00\x00\x0b\xd4\x0b\xd4')
        pos2 = f.tell()
        self.assertEqual(pos2, 2)
        self.assertEqual(f.readframes(2), b'\x17t\x17t"\xad"\xad')
        f.setpos(pos2)
        self.assertEqual(f.readframes(2), b'\x17t\x17t"\xad"\xad')
        f.setpos(pos0)
        self.assertEqual(f.readframes(2), b'\x00\x00\x00\x00\x0b\xd4\x0b\xd4')
        f.close()

    #XXX Need more tests!


def test_main():
    run_unittest(AIFCTest)


if __name__ == "__main__":
    unittest.main()
