from test.support import TESTFN, unlink
import unittest
import pickle
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
        unlink(TESTFN)

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

    def test_getparams(self):
        self.f = sunau.open(TESTFN, 'w')
        self.f.setnchannels(nchannels)
        self.f.setsampwidth(sampwidth)
        self.f.setframerate(framerate)
        self.f.setcomptype('ULAW', '')
        output = b'\0' * nframes * nchannels * sampwidth
        self.f.writeframes(output)
        self.f.close()

        self.f = sunau.open(TESTFN, 'rb')
        params = self.f.getparams()
        self.assertEqual(params.nchannels, nchannels)
        self.assertEqual(params.sampwidth, sampwidth)
        self.assertEqual(params.framerate, framerate)
        self.assertEqual(params.nframes, nframes)
        self.assertEqual(params.comptype, 'ULAW')

        dump = pickle.dumps(params)
        self.assertEqual(pickle.loads(dump), params)

    def test_write_context_manager_calls_close(self):
        # Close checks for a minimum header and will raise an error
        # if it is not set, so this proves that close is called.
        with self.assertRaises(sunau.Error):
            with sunau.open(TESTFN, 'wb') as f:
                pass
        with self.assertRaises(sunau.Error):
            with open(TESTFN, 'wb') as testfile:
                with sunau.open(testfile):
                    pass

    def test_context_manager_with_open_file(self):
        with open(TESTFN, 'wb') as testfile:
            with sunau.open(testfile) as f:
                f.setnchannels(nchannels)
                f.setsampwidth(sampwidth)
                f.setframerate(framerate)
            self.assertFalse(testfile.closed)
        with open(TESTFN, 'rb') as testfile:
            with sunau.open(testfile) as f:
                self.assertFalse(f.getfp().closed)
                params = f.getparams()
                self.assertEqual(params[0], nchannels)
                self.assertEqual(params[1], sampwidth)
                self.assertEqual(params[2], framerate)
            self.assertIsNone(f.getfp())
            self.assertFalse(testfile.closed)

    def test_context_manager_with_filename(self):
        # If the file doesn't get closed, this test won't fail, but it will
        # produce a resource leak warning.
        with sunau.open(TESTFN, 'wb') as f:
            f.setnchannels(nchannels)
            f.setsampwidth(sampwidth)
            f.setframerate(framerate)
        with sunau.open(TESTFN) as f:
            self.assertFalse(f.getfp().closed)
            params = f.getparams()
            self.assertEqual(params[0], nchannels)
            self.assertEqual(params[1], sampwidth)
            self.assertEqual(params[2], framerate)
        self.assertIsNone(f.getfp())


if __name__ == "__main__":
    unittest.main()
