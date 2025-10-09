# Bob Ippolito:
#
# Ok.. the code to find the filename for __getattr__ should look
# something like:
#
# import os
# from macholib.dyld import dyld_find
#
# def find_lib(name):
#      possible = ['lib'+name+'.dylib', name+'.dylib',
#      name+'.framework/'+name]
#      for dylib in possible:
#          try:
#              return os.path.realpath(dyld_find(dylib))
#          except ValueError:
#              pass
#      raise ValueError, "%s not found" % (name,)
#
# It'll have output like this:
#
#  >>> find_lib('pthread')
# '/usr/lib/libSystem.B.dylib'
#  >>> find_lib('z')
# '/usr/lib/libz.1.dylib'
#  >>> find_lib('IOKit')
# '/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit'
#
# -bob

import os
import sys
import unittest

from ctypes.macholib.dyld import dyld_find
from ctypes.macholib.dylib import dylib_info
from ctypes.macholib.framework import framework_info


def find_lib(name):
    possible = ['lib'+name+'.dylib', name+'.dylib', name+'.framework/'+name]
    for dylib in possible:
        try:
            return os.path.realpath(dyld_find(dylib))
        except ValueError:
            pass
    raise ValueError("%s not found" % (name,))


def d(location=None, name=None, shortname=None, version=None, suffix=None):
    return {'location': location, 'name': name, 'shortname': shortname,
            'version': version, 'suffix': suffix}


class MachOTest(unittest.TestCase):
    @unittest.skipUnless(sys.platform == "darwin", 'OSX-specific test')
    def test_find(self):
        self.assertEqual(dyld_find('libSystem.dylib'),
                         '/usr/lib/libSystem.dylib')
        self.assertEqual(dyld_find('System.framework/System'),
                         '/System/Library/Frameworks/System.framework/System')

        # On Mac OS 11, system dylibs are only present in the shared cache,
        # so symlinks like libpthread.dylib -> libSystem.B.dylib will not
        # be resolved by dyld_find
        self.assertIn(find_lib('pthread'),
                              ('/usr/lib/libSystem.B.dylib', '/usr/lib/libpthread.dylib'))

        result = find_lib('z')
        # Issue #21093: dyld default search path includes $HOME/lib and
        # /usr/local/lib before /usr/lib, which caused test failures if
        # a local copy of libz exists in one of them. Now ignore the head
        # of the path.
        self.assertRegex(result, r".*/lib/libz.*\.dylib")

        self.assertIn(find_lib('IOKit'),
                              ('/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit',
                              '/System/Library/Frameworks/IOKit.framework/IOKit'))

    @unittest.skipUnless(sys.platform == "darwin", 'OSX-specific test')
    def test_info(self):
        self.assertIsNone(dylib_info('completely/invalid'))
        self.assertIsNone(dylib_info('completely/invalide_debug'))
        self.assertEqual(dylib_info('P/Foo.dylib'), d('P', 'Foo.dylib', 'Foo'))
        self.assertEqual(dylib_info('P/Foo_debug.dylib'),
                         d('P', 'Foo_debug.dylib', 'Foo', suffix='debug'))
        self.assertEqual(dylib_info('P/Foo.A.dylib'),
                         d('P', 'Foo.A.dylib', 'Foo', 'A'))
        self.assertEqual(dylib_info('P/Foo_debug.A.dylib'),
                         d('P', 'Foo_debug.A.dylib', 'Foo_debug', 'A'))
        self.assertEqual(dylib_info('P/Foo.A_debug.dylib'),
                         d('P', 'Foo.A_debug.dylib', 'Foo', 'A', 'debug'))

    @unittest.skipUnless(sys.platform == "darwin", 'OSX-specific test')
    def test_framework_info(self):
        self.assertIsNone(framework_info('completely/invalid'))
        self.assertIsNone(framework_info('completely/invalid/_debug'))
        self.assertIsNone(framework_info('P/F.framework'))
        self.assertIsNone(framework_info('P/F.framework/_debug'))
        self.assertEqual(framework_info('P/F.framework/F'),
                         d('P', 'F.framework/F', 'F'))
        self.assertEqual(framework_info('P/F.framework/F_debug'),
                         d('P', 'F.framework/F_debug', 'F', suffix='debug'))
        self.assertIsNone(framework_info('P/F.framework/Versions'))
        self.assertIsNone(framework_info('P/F.framework/Versions/A'))
        self.assertEqual(framework_info('P/F.framework/Versions/A/F'),
                         d('P', 'F.framework/Versions/A/F', 'F', 'A'))
        self.assertEqual(framework_info('P/F.framework/Versions/A/F_debug'),
                         d('P', 'F.framework/Versions/A/F_debug', 'F', 'A', 'debug'))


class TestModule(unittest.TestCase):
    def test_deprecated__version__(self):
        import ctypes.macholib

        with self.assertWarnsRegex(
            DeprecationWarning,
            "'__version__' is deprecated and slated for removal in Python 3.20",
        ) as cm:
            getattr(ctypes.macholib, "__version__")
        self.assertEqual(cm.filename, __file__)


if __name__ == "__main__":
    unittest.main()
