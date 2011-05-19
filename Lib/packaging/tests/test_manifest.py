"""Tests for packaging.manifest."""
import os
import logging
from io import StringIO
from packaging.manifest import Manifest

from packaging.tests import unittest, support

_MANIFEST = """\
recursive-include foo *.py   # ok
# nothing here

#

recursive-include bar \\
  *.dat   *.txt
"""

_MANIFEST2 = """\
README
file1
"""


class ManifestTestCase(support.TempdirManager,
                       support.LoggingCatcher,
                       unittest.TestCase):

    def test_manifest_reader(self):
        tmpdir = self.mkdtemp()
        MANIFEST = os.path.join(tmpdir, 'MANIFEST.in')
        with open(MANIFEST, 'w') as f:
            f.write(_MANIFEST)

        manifest = Manifest()
        manifest.read_template(MANIFEST)

        warnings = self.get_logs(logging.WARNING)
        # the manifest should have been read and 3 warnings issued
        # (we didn't provide the files)
        self.assertEqual(3, len(warnings))
        for warning in warnings:
            self.assertIn('no files found matching', warning)

        # reset logs for the next assert
        self.loghandler.flush()

        # manifest also accepts file-like objects
        with open(MANIFEST) as f:
            manifest.read_template(f)

        # the manifest should have been read and 3 warnings issued
        # (we didn't provide the files)
        self.assertEqual(3, len(warnings))

    def test_default_actions(self):
        tmpdir = self.mkdtemp()
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(tmpdir)
        self.write_file('README', 'xxx')
        self.write_file('file1', 'xxx')
        content = StringIO(_MANIFEST2)
        manifest = Manifest()
        manifest.read_template(content)
        self.assertEqual(['README', 'file1'], manifest.files)


def test_suite():
    return unittest.makeSuite(ManifestTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
