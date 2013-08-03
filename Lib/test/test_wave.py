from test.support import TESTFN, unlink
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
        unlink(TESTFN)

    def test_it(self, test_rounding=False):
        self.f = wave.open(TESTFN, 'wb')
        self.f.setnchannels(nchannels)
        self.f.setsampwidth(sampwidth)
        if test_rounding:
            self.f.setframerate(framerate - 0.1)
        else:
            self.f.setframerate(framerate)
        self.f.setnframes(nframes)
        output = b'\0' * nframes * nchannels * sampwidth
        self.f.writeframes(output)
        self.f.close()

        self.f = wave.open(TESTFN, 'rb')
        self.assertEqual(nchannels, self.f.getnchannels())
        self.assertEqual(sampwidth, self.f.getsampwidth())
        self.assertEqual(framerate, self.f.getframerate())
        self.assertEqual(nframes, self.f.getnframes())
        self.assertEqual(self.f.readframes(nframes), output)

    def test_fractional_framerate(self):
        """
        Addresses [ 1512791 ] module wave does no rounding
        Floating point framerates should be rounded, rather than truncated.
        """
        self.test_it(test_rounding=True)

    def test_issue7681(self):
        self.f = wave.open(TESTFN, 'wb')
        self.f.setnchannels(nchannels)
        self.f.setsampwidth(sampwidth)
        self.f.setframerate(framerate)
        # Don't call setnframes, make _write_header divide to figure it out
        output = b'\0' * nframes * nchannels * sampwidth
        self.f.writeframes(output)

    def test_getparams(self):
        self.f = wave.open(TESTFN, 'wb')
        self.f.setnchannels(nchannels)
        self.f.setsampwidth(sampwidth)
        self.f.setframerate(framerate)
        self.f.close()

        self.f = wave.open(TESTFN, 'rb')
        params = self.f.getparams()
        self.assertEqual(params.nchannels, self.f.getnchannels())
        self.assertEqual(params.nframes, self.f.getnframes())
        self.assertEqual(params.sampwidth, self.f.getsampwidth())
        self.assertEqual(params.framerate, self.f.getframerate())
        self.assertEqual(params.comptype, self.f.getcomptype())
        self.assertEqual(params.compname, self.f.getcompname())

    def test_wave_write_context_manager_calls_close(self):
        # Close checks for a minimum header and will raise an error
        # if it is not set, so this proves that close is called.
        with self.assertRaises(wave.Error):
            with wave.open(TESTFN, 'wb') as f:
                pass
        with self.assertRaises(wave.Error):
            with open(TESTFN, 'wb') as testfile:
                with wave.open(testfile):
                    pass

    def test_context_manager_with_open_file(self):
        with open(TESTFN, 'wb') as testfile:
            with wave.open(testfile) as f:
                f.setnchannels(nchannels)
                f.setsampwidth(sampwidth)
                f.setframerate(framerate)
            self.assertFalse(testfile.closed)
        with open(TESTFN, 'rb') as testfile:
            with wave.open(testfile) as f:
                self.assertFalse(f.getfp().closed)
                params = f.getparams()
                self.assertEqual(params.nchannels, nchannels)
                self.assertEqual(params.sampwidth, sampwidth)
                self.assertEqual(params.framerate, framerate)
            self.assertIsNone(f.getfp())
            self.assertFalse(testfile.closed)

    def test_context_manager_with_filename(self):
        # If the file doesn't get closed, this test won't fail, but it will
        # produce a resource leak warning.
        with wave.open(TESTFN, 'wb') as f:
            f.setnchannels(nchannels)
            f.setsampwidth(sampwidth)
            f.setframerate(framerate)
        with wave.open(TESTFN) as f:
            self.assertFalse(f.getfp().closed)
            params = f.getparams()
            self.assertEqual(params.nchannels, nchannels)
            self.assertEqual(params.sampwidth, sampwidth)
            self.assertEqual(params.framerate, framerate)
        self.assertIsNone(f.getfp())


if __name__ == '__main__':
    unittest.main()
