import unittest
from test import audiotests
from test import support
import io
import os
import struct
import sys
import wave


class WaveTest(audiotests.AudioWriteTests,
               audiotests.AudioTestsWithSourceFile):
    module = wave


class WavePCM8Test(WaveTest, unittest.TestCase):
    sndfilename = 'pluck-pcm8.wav'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 1
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = bytes.fromhex("""\
      827F CB80 B184 0088 4B86 C883 3F81 837E 387A 3473 A96B 9A66 \
      6D64 4662 8E60 6F60 D762 7B68 936F 5877 177B 757C 887B 5F7B \
      917A BE7B 3C7C E67F 4F84 C389 418E D192 6E97 0296 FF94 0092 \
      C98E D28D 6F8F 4E8F 648C E38A 888A AB8B D18E 0B91 368E C48A \
      """)


class WavePCM16Test(WaveTest, unittest.TestCase):
    sndfilename = 'pluck-pcm16.wav'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 2
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = bytes.fromhex("""\
      022EFFEA 4B5C00F9 311404EF 80DC0843 CBDF06B2 48AA03F3 BFE701B2 036BFE7C \
      B857FA3E B4B2F34F 2999EBCA 1A5FE6D7 EDFCE491 C626E279 0E05E0B8 EF27E02D \
      5754E275 FB31E843 1373EF89 D827F72C 978BFB7A F5F7FC11 0866FB9C DF30FB42 \
      117FFA36 3EE4FB5D BC75FCB6 66D5FF5F CF16040E 43220978 C1BC0EC8 511F12A4 \
      EEDF1755 82061666 7FFF1446 80001296 499C0EB2 52BA0DB9 EFB70F5C CE400FBC \
      E4B50CEB 63440A5A 08CA0A1F 2BBA0B0B 51460E47 8BCB113C B6F50EEA 44150A59 \
      """)
    if sys.byteorder != 'big':
        frames = wave._byteswap(frames, 2)


class WavePCM24Test(WaveTest, unittest.TestCase):
    sndfilename = 'pluck-pcm24.wav'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 3
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = bytes.fromhex("""\
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
    if sys.byteorder != 'big':
        frames = wave._byteswap(frames, 3)


class WavePCM24ExtTest(WaveTest, unittest.TestCase):
    sndfilename = 'pluck-pcm24-ext.wav'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 3
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = bytes.fromhex("""\
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
    if sys.byteorder != 'big':
        frames = wave._byteswap(frames, 3)


class WavePCM32Test(WaveTest, unittest.TestCase):
    sndfilename = 'pluck-pcm32.wav'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 4
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = bytes.fromhex("""\
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
    if sys.byteorder != 'big':
        frames = wave._byteswap(frames, 4)


class MiscTestCase(unittest.TestCase):
    def test__all__(self):
        not_exported = {'WAVE_FORMAT_PCM', 'WAVE_FORMAT_EXTENSIBLE', 'KSDATAFORMAT_SUBTYPE_PCM'}
        support.check__all__(self, wave, not_exported=not_exported)

    def test_read_deprecations(self):
        filename = support.findfile('pluck-pcm8.wav', subdir='audiodata')
        with wave.open(filename) as reader:
            with self.assertWarns(DeprecationWarning):
                with self.assertRaises(wave.Error):
                    reader.getmark('mark')
            with self.assertWarns(DeprecationWarning):
                self.assertIsNone(reader.getmarkers())

    def test_write_deprecations(self):
        with io.BytesIO(b'') as tmpfile:
            with wave.open(tmpfile, 'wb') as writer:
                writer.setnchannels(1)
                writer.setsampwidth(1)
                writer.setframerate(1)
                writer.setcomptype('NONE', 'not compressed')

                with self.assertWarns(DeprecationWarning):
                    with self.assertRaises(wave.Error):
                        writer.setmark(0, 0, 'mark')
                with self.assertWarns(DeprecationWarning):
                    with self.assertRaises(wave.Error):
                        writer.getmark('mark')
                with self.assertWarns(DeprecationWarning):
                    self.assertIsNone(writer.getmarkers())


class WaveLowLevelTest(unittest.TestCase):

    def test_read_no_chunks(self):
        b = b'SPAM'
        with self.assertRaises(EOFError):
            wave.open(io.BytesIO(b))

    def test_read_no_riff_chunk(self):
        b = b'SPAM' + struct.pack('<L', 0)
        with self.assertRaisesRegex(wave.Error,
                                    'file does not start with RIFF id'):
            wave.open(io.BytesIO(b))

    def test_read_not_wave(self):
        b = b'RIFF' + struct.pack('<L', 4) + b'SPAM'
        with self.assertRaisesRegex(wave.Error,
                                    'not a WAVE file'):
            wave.open(io.BytesIO(b))

    def test_read_no_fmt_no_data_chunk(self):
        b = b'RIFF' + struct.pack('<L', 4) + b'WAVE'
        with self.assertRaisesRegex(wave.Error,
                                    'fmt chunk and/or data chunk missing'):
            wave.open(io.BytesIO(b))

    def test_read_no_data_chunk(self):
        b = b'RIFF' + struct.pack('<L', 28) + b'WAVE'
        b += b'fmt ' + struct.pack('<LHHLLHH', 16, 1, 1, 11025, 11025, 1, 8)
        with self.assertRaisesRegex(wave.Error,
                                    'fmt chunk and/or data chunk missing'):
            wave.open(io.BytesIO(b))

    def test_read_no_fmt_chunk(self):
        b = b'RIFF' + struct.pack('<L', 12) + b'WAVE'
        b += b'data' + struct.pack('<L', 0)
        with self.assertRaisesRegex(wave.Error, 'data chunk before fmt chunk'):
            wave.open(io.BytesIO(b))

    def test_read_wrong_form(self):
        b = b'RIFF' + struct.pack('<L', 36) + b'WAVE'
        b += b'fmt ' + struct.pack('<LHHLLHH', 16, 2, 1, 11025, 11025, 1, 1)
        b += b'data' + struct.pack('<L', 0)
        with self.assertRaisesRegex(wave.Error, 'unknown format: 2'):
            wave.open(io.BytesIO(b))

    def test_read_wrong_number_of_channels(self):
        b = b'RIFF' + struct.pack('<L', 36) + b'WAVE'
        b += b'fmt ' + struct.pack('<LHHLLHH', 16, 1, 0, 11025, 11025, 1, 8)
        b += b'data' + struct.pack('<L', 0)
        with self.assertRaisesRegex(wave.Error, 'bad # of channels'):
            wave.open(io.BytesIO(b))

    def test_read_wrong_sample_width(self):
        b = b'RIFF' + struct.pack('<L', 36) + b'WAVE'
        b += b'fmt ' + struct.pack('<LHHLLHH', 16, 1, 1, 11025, 11025, 1, 0)
        b += b'data' + struct.pack('<L', 0)
        with self.assertRaisesRegex(wave.Error, 'bad sample width'):
            wave.open(io.BytesIO(b))

    def test_open_in_write_raises(self):
        # gh-136523: Wave_write.__del__ should not throw
        with support.catch_unraisable_exception() as cm:
            with self.assertRaises(OSError):
                wave.open(os.curdir, "wb")
            support.gc_collect()
            self.assertIsNone(cm.unraisable)


if __name__ == '__main__':
    unittest.main()
