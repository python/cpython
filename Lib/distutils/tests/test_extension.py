"""Tests for distutils.extension."""
import unittest
import os

from distutils.extension import read_setup_file

class ExtensionTestCase(unittest.TestCase):

    def test_read_setup_file(self):
        # trying to read a Setup file
        # (sample extracted from the PyGame project)
        setup = os.path.join(os.path.dirname(__file__), 'Setup.sample')

        exts = read_setup_file(setup)
        names = [ext.name for ext in exts]
        names.sort()

        # here are the extensions read_setup_file should have created
        # out of the file
        wanted = ['_arraysurfarray', '_camera', '_numericsndarray',
                  '_numericsurfarray', 'base', 'bufferproxy', 'cdrom',
                  'color', 'constants', 'display', 'draw', 'event',
                  'fastevent', 'font', 'gfxdraw', 'image', 'imageext',
                  'joystick', 'key', 'mask', 'mixer', 'mixer_music',
                  'mouse', 'movie', 'overlay', 'pixelarray', 'pypm',
                  'rect', 'rwobject', 'scrap', 'surface', 'surflock',
                  'time', 'transform']

        self.assertEquals(names, wanted)


def test_suite():
    return unittest.makeSuite(ExtensionTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
