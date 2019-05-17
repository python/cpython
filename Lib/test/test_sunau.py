import unittest
from test import audiotests
from audioop import byteswap
import io
import struct
import sys
import sunau


class SunauTest(audiotests.AudioWriteTests,
                audiotests.AudioTestsWithSourceFile):
    module = sunau


class SunauPCM8Test(SunauTest, unittest.TestCase):
    sndfilename = 'pluck-pcm8.au'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 1
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = bytes.fromhex("""\
      02FF 4B00 3104 8008 CB06 4803 BF01 03FE B8FA B4F3 29EB 1AE6 \
      EDE4 C6E2 0EE0 EFE0 57E2 FBE8 13EF D8F7 97FB F5FC 08FB DFFB \
      11FA 3EFB BCFC 66FF CF04 4309 C10E 5112 EE17 8216 7F14 8012 \
      490E 520D EF0F CE0F E40C 630A 080A 2B0B 510E 8B11 B60E 440A \
      """)


class SunauPCM16Test(SunauTest, unittest.TestCase):
    sndfilename = 'pluck-pcm16.au'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 2
    framerate = 11025
    nframes = 48
    comptype = 'NONE'
    compname = 'not compressed'
    frames = bytes.fromhex("""\
      022EFFEA 4B5C00F9 311404EF 80DB0844 CBE006B0 48AB03F3 BFE601B5 0367FE80 \
      B853FA42 B4AFF351 2997EBCD 1A5AE6DC EDF9E492 C627E277 0E06E0B7 EF29E029 \
      5759E271 FB34E83F 1377EF85 D82CF727 978EFB79 F5F7FC12 0864FB9E DF30FB40 \
      1183FA30 3EEAFB59 BC78FCB4 66D5FF60 CF130415 431A097D C1BA0EC7 512312A0 \
      EEE11754 82071666 7FFE1448 80001298 49990EB7 52B40DC1 EFAD0F65 CE3A0FBE \
      E4B70CE6 63490A57 08CC0A1D 2BBC0B09 51480E46 8BCB113C B6F60EE9 44150A5A \
      """)


class SunauPCM24Test(SunauTest, unittest.TestCase):
    sndfilename = 'pluck-pcm24.au'
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


class SunauPCM32Test(SunauTest, unittest.TestCase):
    sndfilename = 'pluck-pcm32.au'
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


class SunauULAWTest(SunauTest, unittest.TestCase):
    sndfilename = 'pluck-ulaw.au'
    sndfilenframes = 3307
    nchannels = 2
    sampwidth = 2
    framerate = 11025
    nframes = 48
    comptype = 'ULAW'
    compname = 'CCITT G.711 u-law'
    frames = bytes.fromhex("""\
      022CFFE8 497C00F4 307C04DC 8284083C CB84069C 497C03DC BE8401AC 036CFE74 \
      B684FA24 B684F344 2A7CEC04 19FCE704 EE04E504 C584E204 0E3CE104 EF04DF84 \
      557CE204 FB24E804 12FCEF04 D784F744 9684FB64 F5C4FC24 083CFBA4 DF84FB24 \
      11FCFA24 3E7CFB64 BA84FCB4 657CFF5C CF84041C 417C09BC C1840EBC 517C12FC \
      EF0416FC 828415FC 7D7C13FC 828412FC 497C0EBC 517C0DBC F0040F3C CD840FFC \
      E5040CBC 617C0A3C 08BC0A3C 2C7C0B3C 517C0E3C 8A8410FC B6840EBC 457C0A3C \
      """)
    if sys.byteorder != 'big':
        frames = byteswap(frames, 2)


class SunauMiscTests(audiotests.AudioMiscTests, unittest.TestCase):
    module = sunau


class SunauLowLevelTest(unittest.TestCase):

    def test_read_bad_magic_number(self):
        b = b'SPA'
        with self.assertRaises(EOFError):
            sunau.open(io.BytesIO(b))
        b = b'SPAM'
        with self.assertRaisesRegex(sunau.Error, 'bad magic number'):
            sunau.open(io.BytesIO(b))

    def test_read_too_small_header(self):
        b = struct.pack('>LLLLL', sunau.AUDIO_FILE_MAGIC, 20, 0,
                        sunau.AUDIO_FILE_ENCODING_LINEAR_8, 11025)
        with self.assertRaisesRegex(sunau.Error, 'header size too small'):
            sunau.open(io.BytesIO(b))

    def test_read_too_large_header(self):
        b = struct.pack('>LLLLLL', sunau.AUDIO_FILE_MAGIC, 124, 0,
                        sunau.AUDIO_FILE_ENCODING_LINEAR_8, 11025, 1)
        b += b'\0' * 100
        with self.assertRaisesRegex(sunau.Error, 'header size ridiculously large'):
            sunau.open(io.BytesIO(b))

    def test_read_wrong_encoding(self):
        b = struct.pack('>LLLLLL', sunau.AUDIO_FILE_MAGIC, 24, 0, 0, 11025, 1)
        with self.assertRaisesRegex(sunau.Error, r'encoding not \(yet\) supported'):
            sunau.open(io.BytesIO(b))

    def test_read_wrong_number_of_channels(self):
        b = struct.pack('>LLLLLL', sunau.AUDIO_FILE_MAGIC, 24, 0,
                        sunau.AUDIO_FILE_ENCODING_LINEAR_8, 11025, 0)
        with self.assertRaisesRegex(sunau.Error, 'bad # of channels'):
            sunau.open(io.BytesIO(b))


if __name__ == "__main__":
    unittest.main()
