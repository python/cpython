#! /usr/bin/env python
"""Test script for the dumbdbm module
   Original by Roger E. Masse
"""

import io
import os
import unittest
import dumbdbm
from test import test_support

_fname = test_support.TESTFN

def _delete_files():
    for ext in [".dir", ".dat", ".bak"]:
        try:
            os.unlink(_fname + ext)
        except OSError:
            pass

class DumbDBMTestCase(unittest.TestCase):
    _dict = {'0': b'',
             'a': b'Python:',
             'b': b'Programming',
             'c': b'the',
             'd': b'way',
             'f': b'Guido',
             'g': b'intended',
             }

    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)

    def test_dumbdbm_creation(self):
        f = dumbdbm.open(_fname, 'c')
        self.assertEqual(list(f.keys()), [])
        for key in self._dict:
            f[key.encode("ascii")] = self._dict[key]
        self.read_helper(f)
        f.close()

    def test_dumbdbm_creation_mode(self):
        # On platforms without chmod, don't do anything.
        if not (hasattr(os, 'chmod') and hasattr(os, 'umask')):
            return

        try:
            old_umask = os.umask(0o002)
            f = dumbdbm.open(_fname, 'c', 0o637)
            f.close()
        finally:
            os.umask(old_umask)

        expected_mode = 0o635
        if os.name != 'posix':
            # Windows only supports setting the read-only attribute.
            # This shouldn't fail, but doesn't work like Unix either.
            expected_mode = 0o666

        import stat
        st = os.stat(_fname + '.dat')
        self.assertEqual(stat.S_IMODE(st.st_mode), expected_mode)
        st = os.stat(_fname + '.dir')
        self.assertEqual(stat.S_IMODE(st.st_mode), expected_mode)

    def test_close_twice(self):
        f = dumbdbm.open(_fname)
        f[b'a'] = b'b'
        self.assertEqual(f[b'a'], b'b')
        f.close()
        f.close()

    def test_dumbdbm_modification(self):
        self.init_db()
        f = dumbdbm.open(_fname, 'w')
        self._dict['g'] = f[b'g'] = b"indented"
        self.read_helper(f)
        f.close()

    def test_dumbdbm_read(self):
        self.init_db()
        f = dumbdbm.open(_fname, 'r')
        self.read_helper(f)
        f.close()

    def test_dumbdbm_keys(self):
        self.init_db()
        f = dumbdbm.open(_fname)
        keys = self.keys_helper(f)
        f.close()

    def test_write_contains(self):
        f = dumbdbm.open(_fname)
        f[b'1'] = b'hello'
        self.assertTrue(b'1' in f)
        f.close()

    def test_write_write_read(self):
        # test for bug #482460
        f = dumbdbm.open(_fname)
        f[b'1'] = b'hello'
        f[b'1'] = b'hello2'
        f.close()
        f = dumbdbm.open(_fname)
        self.assertEqual(f[b'1'], b'hello2')
        f.close()

    def test_line_endings(self):
        # test for bug #1172763: dumbdbm would die if the line endings
        # weren't what was expected.
        f = dumbdbm.open(_fname)
        f[b'1'] = b'hello'
        f[b'2'] = b'hello2'
        f.close()

        # Mangle the file by changing the line separator to Windows or Unix
        data = io.open(_fname + '.dir', 'rb').read()
        if os.linesep == b'\n':
            data = data.replace(b'\n', b'\r\n')
        else:
            data = data.replace(b'\r\n', b'\n')
        io.open(_fname + '.dir', 'wb').write(data)

        f = dumbdbm.open(_fname)
        self.assertEqual(f[b'1'], b'hello')
        self.assertEqual(f[b'2'], b'hello2')


    def read_helper(self, f):
        keys = self.keys_helper(f)
        for key in self._dict:
            self.assertEqual(self._dict[key], f[key.encode("ascii")])

    def init_db(self):
        f = dumbdbm.open(_fname, 'w')
        for k in self._dict:
            f[k.encode("ascii")] = self._dict[k]
        f.close()

    def keys_helper(self, f):
        keys = sorted(k.decode("ascii") for k in f.keys())
        dkeys = sorted(self._dict.keys())
        self.assertEqual(keys, dkeys)
        return keys

    # Perform randomized operations.  This doesn't make assumptions about
    # what *might* fail.
    def test_random(self):
        import random
        d = {}  # mirror the database
        for dummy in range(5):
            f = dumbdbm.open(_fname)
            for dummy in range(100):
                k = random.choice('abcdefghijklm')
                if random.random() < 0.2:
                    if k in d:
                        del d[k]
                        del f[k.encode("ascii")]
                else:
                    v = random.choice((b'a', b'b', b'c')) * random.randrange(10000)
                    d[k] = v
                    f[k.encode("ascii")] = v
                    self.assertEqual(f[k.encode("ascii")], v)
            f.close()

            f = dumbdbm.open(_fname)
            expected = sorted((k.encode("latin-1"), v) for k, v in d.items())
            got = sorted(f.items())
            self.assertEqual(expected, got)
            f.close()

    def tearDown(self):
        _delete_files()

    def setUp(self):
        _delete_files()

def test_main():
    try:
        test_support.run_unittest(DumbDBMTestCase)
    finally:
        _delete_files()

if __name__ == "__main__":
    test_main()
