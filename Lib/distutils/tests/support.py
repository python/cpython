"""Support code for distutils test cases."""

import shutil
import tempfile


class TempdirManager(object):
    """Mix-in class that handles temporary directories for test cases.

    This is intended to be used with unittest.TestCase.
    """

    def setUp(self):
        super(TempdirManager, self).setUp()
        self.tempdirs = []

    def tearDown(self):
        super(TempdirManager, self).tearDown()
        while self.tempdirs:
            d = self.tempdirs.pop()
            shutil.rmtree(d)

    def mkdtemp(self):
        """Create a temporary directory that will be cleaned up.

        Returns the path of the directory.
        """
        d = tempfile.mkdtemp()
        self.tempdirs.append(d)
        return d


class DummyCommand:
    """Class to store options for retrieval via set_undefined_options()."""

    def __init__(self, **kwargs):
        for kw, val in kwargs.items():
            setattr(self, kw, val)

    def ensure_finalized(self):
        pass
