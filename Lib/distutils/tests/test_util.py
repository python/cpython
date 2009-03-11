"""Tests for distutils.util."""
# not covered yet:
#    - byte_compile
#
import os
import sys
import unittest

from distutils.errors import DistutilsPlatformError

from distutils.util import get_platform
from distutils.util import convert_path
from distutils.util import change_root
from distutils.util import check_environ
from distutils.util import split_quoted
from distutils.util import strtobool
from distutils.util import rfc822_escape

from distutils import util # used to patch _environ_checked

class utilTestCase(unittest.TestCase):

    def setUp(self):
        # saving the environment
        self.name = os.name
        self.platform = sys.platform
        self.version = sys.version
        self.sep = os.sep
        self.environ = os.environ
        self.join = os.path.join
        self.isabs = os.path.isabs
        self.splitdrive = os.path.splitdrive

        # patching os.uname
        if hasattr(os, 'uname'):
            self.uname = os.uname
            self._uname = os.uname()
        else:
            self.uname = None
            self._uname = None

        os.uname = self._get_uname

    def tearDown(self):
        # getting back tne environment
        os.name = self.name
        sys.platform = self.platform
        sys.version = self.version
        os.sep = self.sep
        os.environ = self.environ
        os.path.join = self.join
        os.path.isabs = self.isabs
        os.path.splitdrive = self.splitdrive
        if self.uname is not None:
            os.uname = self.uname
        else:
            del os.uname

    def _set_uname(self, uname):
        self._uname = uname

    def _get_uname(self):
        return self._uname

    def test_get_platform(self):

        # windows XP, 32bits
        os.name = 'nt'
        sys.version = ('2.4.4 (#71, Oct 18 2006, 08:34:43) '
                       '[MSC v.1310 32 bit (Intel)]')
        sys.platform = 'win32'
        self.assertEquals(get_platform(), 'win32')

        # windows XP, amd64
        os.name = 'nt'
        sys.version = ('2.4.4 (#71, Oct 18 2006, 08:34:43) '
                       '[MSC v.1310 32 bit (Amd64)]')
        sys.platform = 'win32'
        self.assertEquals(get_platform(), 'win-amd64')

        # windows XP, itanium
        os.name = 'nt'
        sys.version = ('2.4.4 (#71, Oct 18 2006, 08:34:43) '
                       '[MSC v.1310 32 bit (Itanium)]')
        sys.platform = 'win32'
        self.assertEquals(get_platform(), 'win-ia64')

        # macbook
        os.name = 'posix'
        sys.version = ('2.5 (r25:51918, Sep 19 2006, 08:49:13) '
                       '\n[GCC 4.0.1 (Apple Computer, Inc. build 5341)]')
        sys.platform = 'darwin'
        self._set_uname(('Darwin', 'macziade', '8.11.1',
                   ('Darwin Kernel Version 8.11.1: '
                    'Wed Oct 10 18:23:28 PDT 2007; '
                    'root:xnu-792.25.20~1/RELEASE_I386'), 'i386'))
        os.environ['MACOSX_DEPLOYMENT_TARGET'] = '10.3'

        self.assertEquals(get_platform(), 'macosx-10.3-i386')

        # linux debian sarge
        os.name = 'posix'
        sys.version = ('2.3.5 (#1, Jul  4 2007, 17:28:59) '
                       '\n[GCC 4.1.2 20061115 (prerelease) (Debian 4.1.1-21)]')
        sys.platform = 'linux2'
        self._set_uname(('Linux', 'aglae', '2.6.21.1dedibox-r7',
                    '#1 Mon Apr 30 17:25:38 CEST 2007', 'i686'))

        self.assertEquals(get_platform(), 'linux-i686')

        # XXX more platforms to tests here

    def test_convert_path(self):
        # linux/mac
        os.sep = '/'
        def _join(path):
            return '/'.join(path)
        os.path.join = _join

        self.assertEquals(convert_path('/home/to/my/stuff'),
                          '/home/to/my/stuff')

        # win
        os.sep = '\\'
        def _join(*path):
            return '\\'.join(path)
        os.path.join = _join

        self.assertRaises(ValueError, convert_path, '/home/to/my/stuff')
        self.assertRaises(ValueError, convert_path, 'home/to/my/stuff/')

        self.assertEquals(convert_path('home/to/my/stuff'),
                          'home\\to\\my\\stuff')
        self.assertEquals(convert_path('.'),
                          os.curdir)

    def test_change_root(self):
        # linux/mac
        os.name = 'posix'
        def _isabs(path):
            return path[0] == '/'
        os.path.isabs = _isabs
        def _join(*path):
            return '/'.join(path)
        os.path.join = _join

        self.assertEquals(change_root('/root', '/old/its/here'),
                          '/root/old/its/here')
        self.assertEquals(change_root('/root', 'its/here'),
                          '/root/its/here')

        # windows
        os.name = 'nt'
        def _isabs(path):
            return path.startswith('c:\\')
        os.path.isabs = _isabs
        def _splitdrive(path):
            if path.startswith('c:'):
                return ('', path.replace('c:', ''))
            return ('', path)
        os.path.splitdrive = _splitdrive
        def _join(*path):
            return '\\'.join(path)
        os.path.join = _join

        self.assertEquals(change_root('c:\\root', 'c:\\old\\its\\here'),
                          'c:\\root\\old\\its\\here')
        self.assertEquals(change_root('c:\\root', 'its\\here'),
                          'c:\\root\\its\\here')

        # BugsBunny os (it's a great os)
        os.name = 'BugsBunny'
        self.assertRaises(DistutilsPlatformError,
                          change_root, 'c:\\root', 'its\\here')

        # XXX platforms to be covered: os2, mac

    def test_check_environ(self):
        util._environ_checked = 0

        # posix without HOME
        if os.name == 'posix':  # this test won't run on windows
            os.environ = {}
            check_environ()

            import pwd
            self.assertEquals(os.environ['HOME'],
                              pwd.getpwuid(os.getuid())[5])
        else:
            check_environ()

        self.assertEquals(os.environ['PLAT'], get_platform())
        self.assertEquals(util._environ_checked, 1)

    def test_split_quoted(self):
        self.assertEquals(split_quoted('""one"" "two" \'three\' \\four'),
                          ['one', 'two', 'three', 'four'])

    def test_strtobool(self):
        yes = ('y', 'Y', 'yes', 'True', 't', 'true', 'True', 'On', 'on', '1')
        no = ('n', 'no', 'f', 'false', 'off', '0', 'Off', 'No', 'N')

        for y in yes:
            self.assert_(strtobool(y))

        for n in no:
            self.assert_(not strtobool(n))

    def test_rfc822_escape(self):
        header = 'I am a\npoor\nlonesome\nheader\n'
        res = rfc822_escape(header)
        wanted = ('I am a%(8s)spoor%(8s)slonesome%(8s)s'
                  'header%(8s)s') % {'8s': '\n'+8*' '}
        self.assertEquals(res, wanted)

def test_suite():
    return unittest.makeSuite(utilTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
