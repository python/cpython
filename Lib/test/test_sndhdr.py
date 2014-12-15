import sndhdr
import pickle
import unittest
from test.support import findfile

class TestFormats(unittest.TestCase):
    def test_data(self):
        for filename, expected in (
            ('sndhdr.8svx', ('8svx', 0, 1, 0, 8)),
            ('sndhdr.aifc', ('aifc', 44100, 2, 5, 16)),
            ('sndhdr.aiff', ('aiff', 44100, 2, 5, 16)),
            ('sndhdr.au', ('au', 44100, 2, 5.0, 16)),
            ('sndhdr.hcom', ('hcom', 22050.0, 1, -1, 8)),
            ('sndhdr.sndt', ('sndt', 44100, 1, 5, 8)),
            ('sndhdr.voc', ('voc', 0, 1, -1, 8)),
            ('sndhdr.wav', ('wav', 44100, 2, 5, 16)),
        ):
            filename = findfile(filename, subdir="sndhdrdata")
            what = sndhdr.what(filename)
            self.assertNotEqual(what, None, filename)
            self.assertSequenceEqual(what, expected)
            self.assertEqual(what.filetype, expected[0])
            self.assertEqual(what.framerate, expected[1])
            self.assertEqual(what.nchannels, expected[2])
            self.assertEqual(what.nframes, expected[3])
            self.assertEqual(what.sampwidth, expected[4])

    def test_pickleable(self):
        filename = findfile('sndhdr.aifc', subdir="sndhdrdata")
        what = sndhdr.what(filename)
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            dump = pickle.dumps(what, proto)
            self.assertEqual(pickle.loads(dump), what)


if __name__ == '__main__':
    unittest.main()
