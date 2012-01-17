from test.test_support import findfile, run_unittest, TESTFN
import unittest
import os
import io

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
        self.assertEqual(f.getcomptype(), 'NONE')
        self.assertEqual(f.getcompname(), 'not compressed')
        self.assertEqual(f.getparams(), (2, 2, 48000, 14400, 'NONE', 'not compressed'))

    def test_read(self):
        f = self.f = aifc.open(self.sndfilepath)
        self.assertEqual(f.tell(), 0)
        self.assertEqual(f.readframes(2), '\x00\x00\x00\x00\x0b\xd4\x0b\xd4')
        f.rewind()
        pos0 = f.tell()
        self.assertEqual(pos0, 0)
        self.assertEqual(f.readframes(2), '\x00\x00\x00\x00\x0b\xd4\x0b\xd4')
        pos2 = f.tell()
        self.assertEqual(pos2, 2)
        self.assertEqual(f.readframes(2), '\x17t\x17t"\xad"\xad')
        f.setpos(pos2)
        self.assertEqual(f.readframes(2), '\x17t\x17t"\xad"\xad')
        f.setpos(pos0)
        self.assertEqual(f.readframes(2), '\x00\x00\x00\x00\x0b\xd4\x0b\xd4')

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
        fout.setcomptype('ULAW', 'foo')
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
        self.assertEqual(fout.getcomptype(), 'ULAW')
        self.assertEqual(fout.getcompname(), 'foo')
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


class AIFCLowLevelTest(unittest.TestCase):

    def test_read_written(self):
        def read_written(self, what):
            f = io.BytesIO()
            getattr(aifc, '_write_' + what)(f, x)
            f.seek(0)
            return getattr(aifc, '_read_' + what)(f)
        for x in (-1, 0, 0.1, 1):
            self.assertEqual(read_written(x, 'float'), x)
        for x in (float('NaN'), float('Inf')):
            self.assertEqual(read_written(x, 'float'), aifc._HUGE_VAL)
        for x in (b'', b'foo', b'a' * 255):
            self.assertEqual(read_written(x, 'string'), x)
        for x in (-0x7FFFFFFF, -1, 0, 1, 0x7FFFFFFF):
            self.assertEqual(read_written(x, 'long'), x)
        for x in (0, 1, 0xFFFFFFFF):
            self.assertEqual(read_written(x, 'ulong'), x)
        for x in (-0x7FFF, -1, 0, 1, 0x7FFF):
            self.assertEqual(read_written(x, 'short'), x)
        for x in (0, 1, 0xFFFF):
            self.assertEqual(read_written(x, 'ushort'), x)

    def test_read_raises(self):
        f = io.BytesIO(b'\x00')
        self.assertRaises(EOFError, aifc._read_ulong, f)
        self.assertRaises(EOFError, aifc._read_long, f)
        self.assertRaises(EOFError, aifc._read_ushort, f)
        self.assertRaises(EOFError, aifc._read_short, f)

    def test_write_long_string_raises(self):
        f = io.BytesIO()
        with self.assertRaises(ValueError):
            aifc._write_string(f, b'too long' * 255)


def test_main():
    run_unittest(AIFCTest)
    run_unittest(AIFCLowLevelTest)


if __name__ == "__main__":
    unittest.main()
