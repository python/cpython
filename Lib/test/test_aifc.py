from test.test_support import (findfile, TESTFN, unlink, captured_stdout,
                               run_unittest)
import unittest
from test import audiotests
import os
import io
import sys
import struct
import aifc


class AifcTest(audiotests.AudioWriteTests,
               audiotests.AudioTestsWithSourceFile):
    module = aifc
    close_fd = True
    test_unseekable_read = None


class AifcPCM8Test(AifcTest, unittest.TestCase):
    sndfilename = 'pluck-pcm8.aiff'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 1
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = audiotests.fromhex("""\
      02FF 4B00 3104 8008 CB06 4803 BF01 03FE B8FA B4F3 29EB 1AE6 \
      EDE4 C6E2 0EE0 EFE0 57E2 FBE8 13EF D8F7 97FB F5FC 08FB DFFB \
      11FA 3EFB BCFC 66FF CF04 4309 C10E 5112 EE17 8216 7F14 8012 \
      490E 520D EF0F CE0F E40C 630A 080A 2B0B 510E 8B11 B60E 440A \
      """)


class AifcPCM16Test(AifcTest, unittest.TestCase):
    sndfilename = 'pluck-pcm16.aiff'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 2
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = audiotests.fromhex("""\
      022EFFEA 4B5D00F6 311804EA 80E10840 CBE106B1 48A903F5 BFE601B2 036CFE7B \
      B858FA3E B4B1F34F 299AEBCA 1A5DE6DA EDFAE491 C628E275 0E09E0B5 EF2AE029 \
      5758E271 FB35E83F 1376EF86 D82BF727 9790FB76 F5FAFC0F 0867FB9C DF30FB43 \
      117EFA36 3EE5FB5B BC79FCB1 66D9FF5D CF150412 431D097C C1BA0EC8 512112A1 \
      EEE21753 82071665 7FFF1443 8004128F 49A20EAF 52BB0DBA EFB40F60 CE3C0FBF \
      E4B30CEC 63430A5C 08C80A20 2BBB0B08 514A0E43 8BCF1139 B6F60EEB 44120A5E \
      """)


class AifcPCM24Test(AifcTest, unittest.TestCase):
    sndfilename = 'pluck-pcm24.aiff'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 3
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = audiotests.fromhex("""\
      022D65FFEB9D 4B5A0F00FA54 3113C304EE2B 80DCD6084303 \
      CBDEC006B261 48A99803F2F8 BFE82401B07D 036BFBFE7B5D \
      B85756FA3EC9 B4B055F3502B 299830EBCB62 1A5CA7E6D99A \
      EDFA3EE491BD C625EBE27884 0E05A9E0B6CF EF2929E02922 \
      5758D8E27067 FB3557E83E16 1377BFEF8402 D82C5BF7272A \
      978F16FB7745 F5F865FC1013 086635FB9C4E DF30FCFB40EE \
      117FE0FA3438 3EE6B8FB5AC3 BC77A3FCB2F4 66D6DAFF5F32 \
      CF13B9041275 431D69097A8C C1BB600EC74E 5120B912A2BA \
      EEDF641754C0 8207001664B7 7FFFFF14453F 8000001294E6 \
      499C1B0EB3B2 52B73E0DBCA0 EFB2B20F5FD8 CE3CDB0FBE12 \
      E4B49C0CEA2D 6344A80A5A7C 08C8FE0A1FFE 2BB9860B0A0E \
      51486F0E44E1 8BCC64113B05 B6F4EC0EEB36 4413170A5B48 \
      """)


class AifcPCM32Test(AifcTest, unittest.TestCase):
    sndfilename = 'pluck-pcm32.aiff'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 4
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = audiotests.fromhex("""\
      022D65BCFFEB9D92 4B5A0F8000FA549C 3113C34004EE2BC0 80DCD680084303E0 \
      CBDEC0C006B26140 48A9980003F2F8FC BFE8248001B07D92 036BFB60FE7B5D34 \
      B8575600FA3EC920 B4B05500F3502BC0 29983000EBCB6240 1A5CA7A0E6D99A60 \
      EDFA3E80E491BD40 C625EB80E27884A0 0E05A9A0E0B6CFE0 EF292940E0292280 \
      5758D800E2706700 FB3557D8E83E1640 1377BF00EF840280 D82C5B80F7272A80 \
      978F1600FB774560 F5F86510FC101364 086635A0FB9C4E20 DF30FC40FB40EE28 \
      117FE0A0FA3438B0 3EE6B840FB5AC3F0 BC77A380FCB2F454 66D6DA80FF5F32B4 \
      CF13B980041275B0 431D6980097A8C00 C1BB60000EC74E00 5120B98012A2BAA0 \
      EEDF64C01754C060 820700001664B780 7FFFFFFF14453F40 800000001294E6E0 \
      499C1B000EB3B270 52B73E000DBCA020 EFB2B2E00F5FD880 CE3CDB400FBE1270 \
      E4B49CC00CEA2D90 6344A8800A5A7CA0 08C8FE800A1FFEE0 2BB986C00B0A0E00 \
      51486F800E44E190 8BCC6480113B0580 B6F4EC000EEB3630 441317800A5B48A0 \
      """)


class AifcULAWTest(AifcTest, unittest.TestCase):
    sndfilename = 'pluck-ulaw.aifc'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 2
    framerate = 11025
    nframes = 48
    comptype = 'ulaw'
    compname = ''
    frames = audiotests.fromhex("""\
      022CFFE8 497C0104 307C04DC 8284083C CB84069C 497C03DC BE8401AC 036CFE74 \
      B684FA24 B684F344 2A7CEC04 19FCE704 EE04E504 C584E204 0E3CE104 EF04DF84 \
      557CE204 FB24E804 12FCEF04 D784F744 9684FB64 F5C4FC24 083CFBA4 DF84FB24 \
      11FCFA24 3E7CFB64 BA84FCB4 657CFF5C CF84041C 417C093C C1840EBC 517C12FC \
      EF0416FC 828415FC 7D7C13FC 828412FC 497C0EBC 517C0DBC F0040F3C CD840FFC \
      E5040CBC 617C0A3C 08BC0A3C 2C7C0B3C 517C0E3C 8A8410FC B6840EBC 457C0A3C \
      """)
    if sys.byteorder != 'big':
        frames = audiotests.byteswap2(frames)


class AifcMiscTest(audiotests.AudioTests, unittest.TestCase):
    def test_skipunknown(self):
        #Issue 2245
        #This file contains chunk types aifc doesn't recognize.
        self.f = aifc.open(findfile('Sine-1000Hz-300ms.aif'))

    def test_write_markers_values(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        self.assertEqual(fout.getmarkers(), None)
        fout.setmark(1, 0, 'foo1')
        fout.setmark(1, 1, 'foo2')
        self.assertEqual(fout.getmark(1), (1, 1, 'foo2'))
        self.assertEqual(fout.getmarkers(), [(1, 1, 'foo2')])
        fout.initfp(None)

    def test_read_markers(self):
        fout = self.fout = aifc.open(TESTFN, 'wb')
        fout.aiff()
        fout.setparams((1, 1, 1, 1, 'NONE', ''))
        fout.setmark(1, 0, 'odd')
        fout.setmark(2, 0, 'even')
        fout.writeframes('\x00')
        fout.close()
        f = self.f = aifc.open(TESTFN, 'rb')
        self.assertEqual(f.getmarkers(), [(1, 0, 'odd'), (2, 0, 'even')])
        self.assertEqual(f.getmark(1), (1, 0, 'odd'))
        self.assertEqual(f.getmark(2), (2, 0, 'even'))
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
        for x in ('', 'foo', 'a' * 255):
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
        f = io.BytesIO('\x00')
        self.assertRaises(EOFError, aifc._read_ulong, f)
        self.assertRaises(EOFError, aifc._read_long, f)
        self.assertRaises(EOFError, aifc._read_ushort, f)
        self.assertRaises(EOFError, aifc._read_short, f)

    def test_write_long_string_raises(self):
        f = io.BytesIO()
        with self.assertRaises(ValueError):
            aifc._write_string(f, 'too long' * 255)

    def test_wrong_open_mode(self):
        with self.assertRaises(aifc.Error):
            aifc.open(TESTFN, 'wrong_mode')

    def test_read_wrong_form(self):
        b1 = io.BytesIO('WRNG' + struct.pack('>L', 0))
        b2 = io.BytesIO('FORM' + struct.pack('>L', 4) + 'WRNG')
        self.assertRaises(aifc.Error, aifc.open, b1)
        self.assertRaises(aifc.Error, aifc.open, b2)

    def test_read_no_comm_chunk(self):
        b = io.BytesIO('FORM' + struct.pack('>L', 4) + 'AIFF')
        self.assertRaises(aifc.Error, aifc.open, b)

    def test_read_wrong_compression_type(self):
        b = 'FORM' + struct.pack('>L', 4) + 'AIFC'
        b += 'COMM' + struct.pack('>LhlhhLL', 23, 0, 0, 0, 0, 0, 0)
        b += 'WRNG' + struct.pack('B', 0)
        self.assertRaises(aifc.Error, aifc.open, io.BytesIO(b))

    def test_read_wrong_marks(self):
        b = 'FORM' + struct.pack('>L', 4) + 'AIFF'
        b += 'COMM' + struct.pack('>LhlhhLL', 18, 0, 0, 0, 0, 0, 0)
        b += 'SSND' + struct.pack('>L', 8) + '\x00' * 8
        b += 'MARK' + struct.pack('>LhB', 3, 1, 1)
        with captured_stdout() as s:
            f = aifc.open(io.BytesIO(b))
        self.assertEqual(s.getvalue(), 'Warning: MARK chunk contains '
                                       'only 0 markers instead of 1\n')
        self.assertEqual(f.getmarkers(), None)

    def test_read_comm_kludge_compname_even(self):
        b = 'FORM' + struct.pack('>L', 4) + 'AIFC'
        b += 'COMM' + struct.pack('>LhlhhLL', 18, 0, 0, 0, 0, 0, 0)
        b += 'NONE' + struct.pack('B', 4) + 'even' + '\x00'
        b += 'SSND' + struct.pack('>L', 8) + '\x00' * 8
        with captured_stdout() as s:
            f = aifc.open(io.BytesIO(b))
        self.assertEqual(s.getvalue(), 'Warning: bad COMM chunk size\n')
        self.assertEqual(f.getcompname(), 'even')

    def test_read_comm_kludge_compname_odd(self):
        b = 'FORM' + struct.pack('>L', 4) + 'AIFC'
        b += 'COMM' + struct.pack('>LhlhhLL', 18, 0, 0, 0, 0, 0, 0)
        b += 'NONE' + struct.pack('B', 3) + 'odd'
        b += 'SSND' + struct.pack('>L', 8) + '\x00' * 8
        with captured_stdout() as s:
            f = aifc.open(io.BytesIO(b))
        self.assertEqual(s.getvalue(), 'Warning: bad COMM chunk size\n')
        self.assertEqual(f.getcompname(), 'odd')

    def test_write_params_raises(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        wrong_params = (0, 0, 0, 0, 'WRNG', '')
        self.assertRaises(aifc.Error, fout.setparams, wrong_params)
        self.assertRaises(aifc.Error, fout.getparams)
        self.assertRaises(aifc.Error, fout.setnchannels, 0)
        self.assertRaises(aifc.Error, fout.getnchannels)
        self.assertRaises(aifc.Error, fout.setsampwidth, 0)
        self.assertRaises(aifc.Error, fout.getsampwidth)
        self.assertRaises(aifc.Error, fout.setframerate, 0)
        self.assertRaises(aifc.Error, fout.getframerate)
        self.assertRaises(aifc.Error, fout.setcomptype, 'WRNG', '')
        fout.aiff()
        fout.setnchannels(1)
        fout.setsampwidth(1)
        fout.setframerate(1)
        fout.setnframes(1)
        fout.writeframes('\x00')
        self.assertRaises(aifc.Error, fout.setparams, (1, 1, 1, 1, 1, 1))
        self.assertRaises(aifc.Error, fout.setnchannels, 1)
        self.assertRaises(aifc.Error, fout.setsampwidth, 1)
        self.assertRaises(aifc.Error, fout.setframerate, 1)
        self.assertRaises(aifc.Error, fout.setnframes, 1)
        self.assertRaises(aifc.Error, fout.setcomptype, 'NONE', '')
        self.assertRaises(aifc.Error, fout.aiff)
        self.assertRaises(aifc.Error, fout.aifc)

    def test_write_params_singles(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        fout.aifc()
        fout.setnchannels(1)
        fout.setsampwidth(2)
        fout.setframerate(3)
        fout.setnframes(4)
        fout.setcomptype('NONE', 'name')
        self.assertEqual(fout.getnchannels(), 1)
        self.assertEqual(fout.getsampwidth(), 2)
        self.assertEqual(fout.getframerate(), 3)
        self.assertEqual(fout.getnframes(), 0)
        self.assertEqual(fout.tell(), 0)
        self.assertEqual(fout.getcomptype(), 'NONE')
        self.assertEqual(fout.getcompname(), 'name')
        fout.writeframes('\x00' * 4 * fout.getsampwidth() * fout.getnchannels())
        self.assertEqual(fout.getnframes(), 4)
        self.assertEqual(fout.tell(), 4)

    def test_write_params_bunch(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        fout.aifc()
        p = (1, 2, 3, 4, 'NONE', 'name')
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
        for comptype in ('ULAW', 'ulaw', 'ALAW', 'alaw', 'G722'):
            fout = aifc.open(io.BytesIO(), 'wb')
            fout.setsampwidth(1)
            fout.setcomptype(comptype, '')
            self.assertRaises(aifc.Error, fout.close)
            fout.initfp(None)

    def test_write_markers_raises(self):
        fout = aifc.open(io.BytesIO(), 'wb')
        self.assertRaises(aifc.Error, fout.setmark, 0, 0, '')
        self.assertRaises(aifc.Error, fout.setmark, 1, -1, '')
        self.assertRaises(aifc.Error, fout.setmark, 1, 0, None)
        self.assertRaises(aifc.Error, fout.getmark, 1)
        fout.initfp(None)

    def test_write_aiff_by_extension(self):
        sampwidth = 2
        fout = self.fout = aifc.open(TESTFN + '.aiff', 'wb')
        fout.setparams((1, sampwidth, 1, 1, 'ULAW', ''))
        frames = '\x00' * fout.getnchannels() * sampwidth
        fout.writeframes(frames)
        fout.close()
        f = self.f = aifc.open(TESTFN + '.aiff', 'rb')
        self.assertEqual(f.getcomptype(), 'NONE')
        f.close()


def test_main():
    run_unittest(AifcPCM8Test, AifcPCM16Test, AifcPCM16Test, AifcPCM24Test,
                 AifcPCM32Test, AifcULAWTest,
                 AifcMiscTest, AIFCLowLevelTest)

if __name__ == "__main__":
    test_main()
