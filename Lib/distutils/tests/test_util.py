"""Tests for distutils.util."""
import os
import sys
import unittest
from copy import copy
from StringIO import StringIO
import subprocess

from sysconfig import get_config_vars, get_platform
from distutils.errors import DistutilsPlatformError, DistutilsByteCompileError
from distutils.util import (convert_path, change_root,
                            check_environ, split_quoted, strtobool,
                            rfc822_escape, get_compiler_versions,
                            _find_exe_version, _MAC_OS_X_LD_VERSION,
                            byte_compile)
from distutils import util
from distutils.tests import support
from distutils.version import LooseVersion

class FakePopen(object):
    test_class = None
    def __init__(self, cmd, shell, stdout, stderr):
        self.cmd = cmd.split()[0]
        exes = self.test_class._exes
        if self.cmd not in exes:
            # we don't want to call the system, returning an empty
            # output so it doesn't match
            self.stdout = StringIO()
            self.stderr = StringIO()
        else:
            self.stdout = StringIO(exes[self.cmd])
            self.stderr = StringIO()

class UtilTestCase(support.EnvironGuard, unittest.TestCase):

    def setUp(self):
        super(UtilTestCase, self).setUp()
        # saving the environment
        self.name = os.name
        self.platform = sys.platform
        self.version = sys.version
        self.sep = os.sep
        self.join = os.path.join
        self.isabs = os.path.isabs
        self.splitdrive = os.path.splitdrive
        #self._config_vars = copy(sysconfig._config_vars)

        # patching os.uname
        if hasattr(os, 'uname'):
            self.uname = os.uname
            self._uname = os.uname()
        else:
            self.uname = None
            self._uname = None
        os.uname = self._get_uname

        # patching POpen
        self.old_find_executable = util.find_executable
        util.find_executable = self._find_executable
        self._exes = {}
        self.old_popen = subprocess.Popen
        self.old_stdout  = sys.stdout
        self.old_stderr = sys.stderr
        FakePopen.test_class = self
        subprocess.Popen = FakePopen

    def tearDown(self):
        # getting back the environment
        os.name = self.name
        sys.platform = self.platform
        sys.version = self.version
        os.sep = self.sep
        os.path.join = self.join
        os.path.isabs = self.isabs
        os.path.splitdrive = self.splitdrive
        if self.uname is not None:
            os.uname = self.uname
        else:
            del os.uname
        #sysconfig._config_vars = copy(self._config_vars)
        util.find_executable = self.old_find_executable
        subprocess.Popen = self.old_popen
        sys.old_stdout  = self.old_stdout
        sys.old_stderr = self.old_stderr
        super(UtilTestCase, self).tearDown()

    def _set_uname(self, uname):
        self._uname = uname

    def _get_uname(self):
        return self._uname

    def test_get_platform(self):
        platform = util.get_platform()
        self.assertEquals(platform, get_platform())
        util.set_platform('MyOwnPlatform')
        self.assertEquals('MyOwnPlatform', util.get_platform())
        util.set_platform(platform)

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
        if 'HOME' in os.environ:
            del os.environ['HOME']

        # posix without HOME
        if os.name == 'posix':  # this test won't run on windows
            check_environ()
            import pwd
            self.assertEquals(os.environ['HOME'], pwd.getpwuid(os.getuid())[5])
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
            self.assertTrue(strtobool(y))

        for n in no:
            self.assertTrue(not strtobool(n))

    def test_rfc822_escape(self):
        header = 'I am a\npoor\nlonesome\nheader\n'
        res = rfc822_escape(header)
        wanted = ('I am a%(8s)spoor%(8s)slonesome%(8s)s'
                  'header%(8s)s') % {'8s': '\n'+8*' '}
        self.assertEquals(res, wanted)

    def test_find_exe_version(self):
        # the ld version scheme under MAC OS is:
        #   ^@(#)PROGRAM:ld  PROJECT:ld64-VERSION
        #
        # where VERSION is a 2-digit number for major
        # revisions. For instance under Leopard, it's
        # currently 77
        #
        # Dots are used when branching is done.
        #
        # The SnowLeopard ld64 is currently 95.2.12

        for output, version in (('@(#)PROGRAM:ld  PROJECT:ld64-77', '77'),
                                ('@(#)PROGRAM:ld  PROJECT:ld64-95.2.12',
                                 '95.2.12')):
            result = _MAC_OS_X_LD_VERSION.search(output)
            self.assertEquals(result.group(1), version)

    def _find_executable(self, name):
        if name in self._exes:
            return name
        return None

    def test_get_compiler_versions(self):
        # get_versions calls distutils.spawn.find_executable on
        # 'gcc', 'ld' and 'dllwrap'
        self.assertEquals(get_compiler_versions(), (None, None, None))

        # Let's fake we have 'gcc' and it returns '3.4.5'
        self._exes['gcc'] = 'gcc (GCC) 3.4.5 (mingw special)\nFSF'
        res = get_compiler_versions()
        self.assertEquals(str(res[0]), '3.4.5')

        # and let's see what happens when the version
        # doesn't match the regular expression
        # (\d+\.\d+(\.\d+)*)
        self._exes['gcc'] = 'very strange output'
        res = get_compiler_versions()
        self.assertEquals(res[0], None)

        # same thing for ld
        if sys.platform != 'darwin':
            self._exes['ld'] = 'GNU ld version 2.17.50 20060824'
            res = get_compiler_versions()
            self.assertEquals(str(res[1]), '2.17.50')
            self._exes['ld'] = '@(#)PROGRAM:ld  PROJECT:ld64-77'
            res = get_compiler_versions()
            self.assertEquals(res[1], None)
        else:
            self._exes['ld'] = 'GNU ld version 2.17.50 20060824'
            res = get_compiler_versions()
            self.assertEquals(res[1], None)
            self._exes['ld'] = '@(#)PROGRAM:ld  PROJECT:ld64-77'
            res = get_compiler_versions()
            self.assertEquals(str(res[1]), '77')

        # and dllwrap
        self._exes['dllwrap'] = 'GNU dllwrap 2.17.50 20060824\nFSF'
        res = get_compiler_versions()
        self.assertEquals(str(res[2]), '2.17.50')
        self._exes['dllwrap'] = 'Cheese Wrap'
        res = get_compiler_versions()
        self.assertEquals(res[2], None)

    def test_dont_write_bytecode(self):
        # makes sure byte_compile raise a DistutilsError
        # if sys.dont_write_bytecode is True
        old_dont_write_bytecode = sys.dont_write_bytecode
        sys.dont_write_bytecode = True
        try:
            self.assertRaises(DistutilsByteCompileError, byte_compile, [])
        finally:
            sys.dont_write_bytecode = old_dont_write_bytecode

def test_suite():
    return unittest.makeSuite(UtilTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
