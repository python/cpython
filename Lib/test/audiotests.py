from test.test_support import findfile, TESTFN, unlink
import unittest
import array
import io
import pickle
import sys
import base64

def fromhex(s):
    return base64.b16decode(s.replace(' ', ''))

def byteswap2(data):
    a = array.array('h', data)
    a.byteswap()
    return a.tostring()

def byteswap3(data):
    ba = bytearray(data)
    ba[::3] = data[2::3]
    ba[2::3] = data[::3]
    return bytes(ba)

def byteswap4(data):
    a = array.array('i', data)
    a.byteswap()
    return a.tostring()


class AudioTests:
    close_fd = False

    def setUp(self):
        self.f = self.fout = None

    def tearDown(self):
        if self.f is not None:
            self.f.close()
        if self.fout is not None:
            self.fout.close()
        unlink(TESTFN)

    def check_params(self, f, nchannels, sampwidth, framerate, nframes,
                     comptype, compname):
        self.assertEqual(f.getnchannels(), nchannels)
        self.assertEqual(f.getsampwidth(), sampwidth)
        self.assertEqual(f.getframerate(), framerate)
        self.assertEqual(f.getnframes(), nframes)
        self.assertEqual(f.getcomptype(), comptype)
        self.assertEqual(f.getcompname(), compname)

        params = f.getparams()
        self.assertEqual(params,
                (nchannels, sampwidth, framerate, nframes, comptype, compname))

        dump = pickle.dumps(params)
        self.assertEqual(pickle.loads(dump), params)


class AudioWriteTests(AudioTests):

    def create_file(self, testfile):
        f = self.fout = self.module.open(testfile, 'wb')
        f.setnchannels(self.nchannels)
        f.setsampwidth(self.sampwidth)
        f.setframerate(self.framerate)
        f.setcomptype(self.comptype, self.compname)
        return f

    def check_file(self, testfile, nframes, frames):
        f = self.module.open(testfile, 'rb')
        try:
            self.assertEqual(f.getnchannels(), self.nchannels)
            self.assertEqual(f.getsampwidth(), self.sampwidth)
            self.assertEqual(f.getframerate(), self.framerate)
            self.assertEqual(f.getnframes(), nframes)
            self.assertEqual(f.readframes(nframes), frames)
        finally:
            f.close()

    def test_write_params(self):
        f = self.create_file(TESTFN)
        f.setnframes(self.nframes)
        f.writeframes(self.frames)
        self.check_params(f, self.nchannels, self.sampwidth, self.framerate,
                          self.nframes, self.comptype, self.compname)
        f.close()

    def test_write(self):
        f = self.create_file(TESTFN)
        f.setnframes(self.nframes)
        f.writeframes(self.frames)
        f.close()

        self.check_file(TESTFN, self.nframes, self.frames)

    def test_incompleted_write(self):
        with open(TESTFN, 'wb') as testfile:
            testfile.write(b'ababagalamaga')
            f = self.create_file(testfile)
            f.setnframes(self.nframes + 1)
            f.writeframes(self.frames)
            f.close()

        with open(TESTFN, 'rb') as testfile:
            self.assertEqual(testfile.read(13), b'ababagalamaga')
            self.check_file(testfile, self.nframes, self.frames)

    def test_multiple_writes(self):
        with open(TESTFN, 'wb') as testfile:
            testfile.write(b'ababagalamaga')
            f = self.create_file(testfile)
            f.setnframes(self.nframes)
            framesize = self.nchannels * self.sampwidth
            f.writeframes(self.frames[:-framesize])
            f.writeframes(self.frames[-framesize:])
            f.close()

        with open(TESTFN, 'rb') as testfile:
            self.assertEqual(testfile.read(13), b'ababagalamaga')
            self.check_file(testfile, self.nframes, self.frames)

    def test_overflowed_write(self):
        with open(TESTFN, 'wb') as testfile:
            testfile.write(b'ababagalamaga')
            f = self.create_file(testfile)
            f.setnframes(self.nframes - 1)
            f.writeframes(self.frames)
            f.close()

        with open(TESTFN, 'rb') as testfile:
            self.assertEqual(testfile.read(13), b'ababagalamaga')
            self.check_file(testfile, self.nframes, self.frames)


class AudioTestsWithSourceFile(AudioTests):

    @classmethod
    def setUpClass(cls):
        cls.sndfilepath = findfile(cls.sndfilename, subdir='audiodata')

    def test_read_params(self):
        f = self.f = self.module.open(self.sndfilepath)
        #self.assertEqual(f.getfp().name, self.sndfilepath)
        self.check_params(f, self.nchannels, self.sampwidth, self.framerate,
                          self.sndfilenframes, self.comptype, self.compname)

    def test_close(self):
        with open(self.sndfilepath, 'rb') as testfile:
            f = self.f = self.module.open(testfile)
            self.assertFalse(testfile.closed)
            f.close()
            self.assertEqual(testfile.closed, self.close_fd)
        with open(TESTFN, 'wb') as testfile:
            fout = self.fout = self.module.open(testfile, 'wb')
            self.assertFalse(testfile.closed)
            with self.assertRaises(self.module.Error):
                fout.close()
            self.assertEqual(testfile.closed, self.close_fd)
            fout.close() # do nothing

    def test_read(self):
        framesize = self.nchannels * self.sampwidth
        chunk1 = self.frames[:2 * framesize]
        chunk2 = self.frames[2 * framesize: 4 * framesize]
        f = self.f = self.module.open(self.sndfilepath)
        self.assertEqual(f.readframes(0), b'')
        self.assertEqual(f.tell(), 0)
        self.assertEqual(f.readframes(2), chunk1)
        f.rewind()
        pos0 = f.tell()
        self.assertEqual(pos0, 0)
        self.assertEqual(f.readframes(2), chunk1)
        pos2 = f.tell()
        self.assertEqual(pos2, 2)
        self.assertEqual(f.readframes(2), chunk2)
        f.setpos(pos2)
        self.assertEqual(f.readframes(2), chunk2)
        f.setpos(pos0)
        self.assertEqual(f.readframes(2), chunk1)
        with self.assertRaises(self.module.Error):
            f.setpos(-1)
        with self.assertRaises(self.module.Error):
            f.setpos(f.getnframes() + 1)

    def test_copy(self):
        f = self.f = self.module.open(self.sndfilepath)
        fout = self.fout = self.module.open(TESTFN, 'wb')
        fout.setparams(f.getparams())
        i = 0
        n = f.getnframes()
        while n > 0:
            i += 1
            fout.writeframes(f.readframes(i))
            n -= i
        fout.close()
        fout = self.fout = self.module.open(TESTFN, 'rb')
        f.rewind()
        self.assertEqual(f.getparams(), fout.getparams())
        self.assertEqual(f.readframes(f.getnframes()),
                         fout.readframes(fout.getnframes()))

    def test_read_not_from_start(self):
        with open(TESTFN, 'wb') as testfile:
            testfile.write(b'ababagalamaga')
            with open(self.sndfilepath, 'rb') as f:
                testfile.write(f.read())

        with open(TESTFN, 'rb') as testfile:
            self.assertEqual(testfile.read(13), b'ababagalamaga')
            f = self.module.open(testfile, 'rb')
            try:
                self.assertEqual(f.getnchannels(), self.nchannels)
                self.assertEqual(f.getsampwidth(), self.sampwidth)
                self.assertEqual(f.getframerate(), self.framerate)
                self.assertEqual(f.getnframes(), self.sndfilenframes)
                self.assertEqual(f.readframes(self.nframes), self.frames)
            finally:
                f.close()
