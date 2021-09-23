import os
import sys
import unittest

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

from ctypes.macholib.dyld import dyld_find

def find_lib(name):
    possible = ['lib'+name+'.dylib', name+'.dylib', name+'.framework/'+name]
    for dylib in possible:
        try:
            return os.path.realpath(dyld_find(dylib))
        except ValueError:
            pass
    raise ValueError("%s not found" % (name,))

class MachOTest(unittest.TestCase):
    @unittest.skipUnless(sys.platform == "darwin", 'OSX-specific test')
    def test_find(self):
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

if __name__ == "__main__":
    unittest.main()
