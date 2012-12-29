from test.support import findfile, run_unittest, TESTFN, unlink
import unittest
import os
import io
import struct

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
        unlink(TESTFN)
        unlink(TESTFN + '.aiff')

    def test_skipunknown(self):
        #Issue 2245
        #This file contains chunk types aifc doesn't recognize.
        self.f = aifc.open(self.sndfilepath)

    def test_params(self):
        f = self.f = aifc.open(self.sndfilepath)
        self.assertEqual(f.getfp().name, self.sndfilepath)
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
        self.assertEqual(f.readframes(0), b'')
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
        with self.assertRaises(aifc.Error):
            f.setpos(-1)
        with self.assertRaises(aifc.Error):
            f.setpos(f.getnframes() + 1)

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
        testfile = open(TESTFN, 'wb')
        fout = aifc.open(testfile, 'wb')
        self.assertFalse(testfile.closed)
        with self.assertRaises(aifc.Error):
            fout.close()
        self.assertTrue(testfile.closed)
        fout.close() # do nothing

    def test_write_header_comptype_sampwidth(self):
        for comptype in (b'ULAW', b'ulaw', b'ALAW', b'alaw', b'G722'):
            fout = aifc.open(io.BytesIO(), 'wb')
            fout.setnchannels(1)
            fout.setframerate(1)
            fout.setcomptype(comptype, b'')
            fout.close()
            self.assertEqual(fout.getsampwidth(), 2)
            fout.initfp(None)

    def test_write_markers_values(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        self.assertEqual(fout.getmarkers(), None)
        fout.setmark(1, 0, b'foo1')
        fout.setmark(1, 1, b'foo2')
        self.assertEqual(fout.getmark(1), (1, 1, b'foo2'))
        self.assertEqual(fout.getmarkers(), [(1, 1, b'foo2')])
        fout.initfp(None)

    def test_read_markers(self):
        fout = self.fout = aifc.open(TESTFN, 'wb')
        fout.aiff()
        fout.setparams((1, 1, 1, 1, b'NONE', b''))
        fout.setmark(1, 0, b'odd')
        fout.setmark(2, 0, b'even')
        fout.writeframes(b'\x00')
        fout.close()
        f = self.f = aifc.open(TESTFN, 'rb')
        self.assertEqual(f.getmarkers(), [(1, 0, b'odd'), (2, 0, b'even')])
        self.assertEqual(f.getmark(1), (1, 0, b'odd'))
        self.assertEqual(f.getmark(2), (2, 0, b'even'))
        self.assertRaises(aifc.Error, f.getmark, 3)


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

    def test_wrong_open_mode(self):
        with self.assertRaises(aifc.Error):
            aifc.open(TESTFN, 'wrong_mode')

    def test_read_wrong_form(self):
        b1 = io.BytesIO(b'WRNG' + struct.pack('>L', 0))
        b2 = io.BytesIO(b'FORM' + struct.pack('>L', 4) + b'WRNG')
        self.assertRaises(aifc.Error, aifc.open, b1)
        self.assertRaises(aifc.Error, aifc.open, b2)

    def test_read_no_comm_chunk(self):
        b = io.BytesIO(b'FORM' + struct.pack('>L', 4) + b'AIFF')
        self.assertRaises(aifc.Error, aifc.open, b)

    def test_read_wrong_compression_type(self):
        b = b'FORM' + struct.pack('>L', 4) + b'AIFC'
        b += b'COMM' + struct.pack('>LhlhhLL', 23, 0, 0, 0, 0, 0, 0)
        b += b'WRNG' + struct.pack('B', 0)
        self.assertRaises(aifc.Error, aifc.open, io.BytesIO(b))

    def test_read_wrong_marks(self):
        b = b'FORM' + struct.pack('>L', 4) + b'AIFF'
        b += b'COMM' + struct.pack('>LhlhhLL', 18, 0, 0, 0, 0, 0, 0)
        b += b'SSND' + struct.pack('>L', 8) + b'\x00' * 8
        b += b'MARK' + struct.pack('>LhB', 3, 1, 1)
        with self.assertWarns(UserWarning):
            f = aifc.open(io.BytesIO(b))
        self.assertEqual(f.getmarkers(), None)

    def test_read_comm_kludge_compname_even(self):
        b = b'FORM' + struct.pack('>L', 4) + b'AIFC'
        b += b'COMM' + struct.pack('>LhlhhLL', 18, 0, 0, 0, 0, 0, 0)
        b += b'NONE' + struct.pack('B', 4) + b'even' + b'\x00'
        b += b'SSND' + struct.pack('>L', 8) + b'\x00' * 8
        with self.assertWarns(UserWarning):
            f = aifc.open(io.BytesIO(b))
        self.assertEqual(f.getcompname(), b'even')

    def test_read_comm_kludge_compname_odd(self):
        b = b'FORM' + struct.pack('>L', 4) + b'AIFC'
        b += b'COMM' + struct.pack('>LhlhhLL', 18, 0, 0, 0, 0, 0, 0)
        b += b'NONE' + struct.pack('B', 3) + b'odd'
        b += b'SSND' + struct.pack('>L', 8) + b'\x00' * 8
        with self.assertWarns(UserWarning):
            f = aifc.open(io.BytesIO(b))
        self.assertEqual(f.getcompname(), b'odd')

    def test_write_params_raises(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        wrong_params = (0, 0, 0, 0, b'WRNG', '')
        self.assertRaises(aifc.Error, fout.setparams, wrong_params)
        self.assertRaises(aifc.Error, fout.getparams)
        self.assertRaises(aifc.Error, fout.setnchannels, 0)
        self.assertRaises(aifc.Error, fout.getnchannels)
        self.assertRaises(aifc.Error, fout.setsampwidth, 0)
        self.assertRaises(aifc.Error, fout.getsampwidth)
        self.assertRaises(aifc.Error, fout.setframerate, 0)
        self.assertRaises(aifc.Error, fout.getframerate)
        self.assertRaises(aifc.Error, fout.setcomptype, b'WRNG', '')
        fout.aiff()
        fout.setnchannels(1)
        fout.setsampwidth(1)
        fout.setframerate(1)
        fout.setnframes(1)
        fout.writeframes(b'\x00')
        self.assertRaises(aifc.Error, fout.setparams, (1, 1, 1, 1, 1, 1))
        self.assertRaises(aifc.Error, fout.setnchannels, 1)
        self.assertRaises(aifc.Error, fout.setsampwidth, 1)
        self.assertRaises(aifc.Error, fout.setframerate, 1)
        self.assertRaises(aifc.Error, fout.setnframes, 1)
        self.assertRaises(aifc.Error, fout.setcomptype, b'NONE', '')
        self.assertRaises(aifc.Error, fout.aiff)
        self.assertRaises(aifc.Error, fout.aifc)

    def test_write_params_singles(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        fout.aifc()
        fout.setnchannels(1)
        fout.setsampwidth(2)
        fout.setframerate(3)
        fout.setnframes(4)
        fout.setcomptype(b'NONE', b'name')
        self.assertEqual(fout.getnchannels(), 1)
        self.assertEqual(fout.getsampwidth(), 2)
        self.assertEqual(fout.getframerate(), 3)
        self.assertEqual(fout.getnframes(), 0)
        self.assertEqual(fout.tell(), 0)
        self.assertEqual(fout.getcomptype(), b'NONE')
        self.assertEqual(fout.getcompname(), b'name')
        fout.writeframes(b'\x00' * 4 * fout.getsampwidth() * fout.getnchannels())
        self.assertEqual(fout.getnframes(), 4)
        self.assertEqual(fout.tell(), 4)

    def test_write_params_bunch(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        fout.aifc()
        p = (1, 2, 3, 4, b'NONE', b'name')
        fout.setparams(p)
        self.assertEqual(fout.getparams(), p)
        fout.initfp(None)

    def test_write_header_raises(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        self.assertRaises(aifc.Error, fout.close)
        fout = aifc.open(io.BytesIO(), 'wb')
        fout.setnchannels(1)
        self.assertRaises(aifc.Error, fout.close)
        fout = aifc.open(io.BytesIO(), 'wb')
        fout.setnchannels(1)
        fout.setsampwidth(1)
        self.assertRaises(aifc.Error, fout.close)

    def test_write_header_comptype_raises(self):
        for comptype in (b'ULAW', b'ulaw', b'ALAW', b'alaw', b'G722'):
            fout = aifc.open(io.BytesIO(), 'wb')
            fout.setsampwidth(1)
            fout.setcomptype(comptype, b'')
            self.assertRaises(aifc.Error, fout.close)
            fout.initfp(None)

    def test_write_markers_raises(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        self.assertRaises(aifc.Error, fout.setmark, 0, 0, b'')
        self.assertRaises(aifc.Error, fout.setmark, 1, -1, b'')
        self.assertRaises(aifc.Error, fout.setmark, 1, 0, None)
        self.assertRaises(aifc.Error, fout.getmark, 1)
        fout.initfp(None)

    def test_write_aiff_by_extension(self):
        sampwidth = 2
        fout = self.fout = aifc.open(TESTFN + '.aiff', 'wb')
        fout.setparams((1, sampwidth, 1, 1, b'ULAW', b''))
        frames = b'\x00' * fout.getnchannels() * sampwidth
        fout.writeframes(frames)
        fout.close()
        f = self.f = aifc.open(TESTFN + '.aiff', 'rb')
        self.assertEqual(f.getcomptype(), b'NONE')
        f.close()


def test_main():
    run_unittest(AIFCTest)
    run_unittest(AIFCLowLevelTest)


if __name__ == "__main__":
    unittest.main()
