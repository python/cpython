from test.support import findfile, run_unittest, TESTFN
import unittest
import os

import aifc


class AIFCTest(unittest.TestCase):

    def setUp(self):
        self.f = self.fout = None
        self.sndfilepath = findfile('Sine-1000Hz-300ms.aif')

    def tearDown(self):
        if self.f is not None:
            self.f.close()
        if self.fout is not None:
            try:
                self.fout.close()
            except (aifc.Error, AttributeError):
                pass
        try:
            os.remove(TESTFN)
        except OSError:
            pass

    def test_skipunknown(self):
        #Issue 2245
        #This file contains chunk types aifc doesn't recognize.
        self.f = aifc.open(self.sndfilepath)

    def test_params(self):
        f = self.f = aifc.open(self.sndfilepath)
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

    def test_read(self):
        f = self.f = aifc.open(self.sndfilepath)
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

    def test_write(self):
        f = self.f = aifc.open(self.sndfilepath)
        fout = self.fout = aifc.open(TESTFN, 'wb')
        fout.aifc()
        fout.setparams(f.getparams())
        for frame in range(f.getnframes()):
            fout.writeframes(f.readframes(1))
        fout.close()
        fout = self.fout = aifc.open(TESTFN, 'rb')
        f.rewind()
        self.assertEqual(f.getparams(), fout.getparams())
        self.assertEqual(f.readframes(5), fout.readframes(5))

    def test_compress(self):
        f = self.f = aifc.open(self.sndfilepath)
        fout = self.fout = aifc.open(TESTFN, 'wb')
        fout.aifc()
        fout.setnchannels(f.getnchannels())
        fout.setsampwidth(f.getsampwidth())
        fout.setframerate(f.getframerate())
        fout.setcomptype(b'ULAW', b'foo')
        for frame in range(f.getnframes()):
            fout.writeframes(f.readframes(1))
        fout.close()
        self.assertLess(
            os.stat(TESTFN).st_size,
            os.stat(self.sndfilepath).st_size*0.75,
            )
        fout = self.fout = aifc.open(TESTFN, 'rb')
        f.rewind()
        self.assertEqual(f.getparams()[0:3], fout.getparams()[0:3])
        self.assertEqual(fout.getcomptype(), b'ULAW')
        self.assertEqual(fout.getcompname(), b'foo')
        # XXX: this test fails, not sure if it should succeed or not
        # self.assertEqual(f.readframes(5), fout.readframes(5))

    def test_close(self):
        class Wrapfile(object):
            def __init__(self, file):
                self.file = open(file, 'rb')
                self.closed = False
            def close(self):
                self.file.close()
                self.closed = True
            def __getattr__(self, attr): return getattr(self.file, attr)
        testfile = Wrapfile(self.sndfilepath)
        f = self.f = aifc.open(testfile)
        self.assertEqual(testfile.closed, False)
        f.close()
        self.assertEqual(testfile.closed, True)


def test_main():
    run_unittest(AIFCTest)


if __name__ == "__main__":
    unittest.main()
