from unittest import mock
import os
import platform
import subprocess
import sys
import sysconfig
import tempfile
import unittest
import warnings

from test import support

class PlatformTest(unittest.TestCase):
    def test_architecture(self):
        res = platform.architecture()

    @support.skip_unless_symlink
    def test_architecture_via_symlink(self): # issue3762
        if sys.platform == "win32" and not os.path.exists(sys.executable):
            # App symlink appears to not exist, but we want the
            # real executable here anyway
            import _winapi
            real = _winapi.GetModuleFileName(0)
        else:
            real = os.path.realpath(sys.executable)
        link = os.path.abspath(support.TESTFN)
        os.symlink(real, link)

        # On Windows, the EXE needs to know where pythonXY.dll and *.pyd is at
        # so we add the directory to the path, PYTHONHOME and PYTHONPATH.
        env = None
        if sys.platform == "win32":
            env = {k.upper(): os.environ[k] for k in os.environ}
            env["PATH"] = "{};{}".format(
                os.path.dirname(real), env.get("PATH", ""))
            env["PYTHONHOME"] = os.path.dirname(real)
            if sysconfig.is_python_build(True):
                env["PYTHONPATH"] = os.path.dirname(os.__file__)

        def get(python, env=None):
            cmd = [python, '-c',
                'import platform; print(platform.architecture())']
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, env=env)
            r = p.communicate()
            if p.returncode:
                print(repr(r[0]))
                print(repr(r[1]), file=sys.stderr)
                self.fail('unexpected return code: {0} (0x{0:08X})'
                          .format(p.returncode))
            return r

        try:
            self.assertEqual(get(sys.executable), get(link, env=env))
        finally:
            os.remove(link)

    def test_platform(self):
        for aliased in (False, True):
            for terse in (False, True):
                res = platform.platform(aliased, terse)

    def test_system(self):
        res = platform.system()

    def test_node(self):
        res = platform.node()

    def test_release(self):
        res = platform.release()

    def test_version(self):
        res = platform.version()

    def test_machine(self):
        res = platform.machine()

    def test_processor(self):
        res = platform.processor()

    def setUp(self):
        self.save_version = sys.version
        self.save_git = sys._git
        self.save_platform = sys.platform

    def tearDown(self):
        sys.version = self.save_version
        sys._git = self.save_git
        sys.platform = self.save_platform

    def test_sys_version(self):
        # Old test.
        for input, output in (
            ('2.4.3 (#1, Jun 21 2006, 13:54:21) \n[GCC 3.3.4 (pre 3.3.5 20040809)]',
             ('CPython', '2.4.3', '', '', '1', 'Jun 21 2006 13:54:21', 'GCC 3.3.4 (pre 3.3.5 20040809)')),
            ('IronPython 1.0.60816 on .NET 2.0.50727.42',
             ('IronPython', '1.0.60816', '', '', '', '', '.NET 2.0.50727.42')),
            ('IronPython 1.0 (1.0.61005.1977) on .NET 2.0.50727.42',
             ('IronPython', '1.0.0', '', '', '', '', '.NET 2.0.50727.42')),
            ('2.4.3 (truncation, date, t) \n[GCC]',
             ('CPython', '2.4.3', '', '', 'truncation', 'date t', 'GCC')),
            ('2.4.3 (truncation, date, ) \n[GCC]',
             ('CPython', '2.4.3', '', '', 'truncation', 'date', 'GCC')),
            ('2.4.3 (truncation, date,) \n[GCC]',
             ('CPython', '2.4.3', '', '', 'truncation', 'date', 'GCC')),
            ('2.4.3 (truncation, date) \n[GCC]',
             ('CPython', '2.4.3', '', '', 'truncation', 'date', 'GCC')),
            ('2.4.3 (truncation, d) \n[GCC]',
             ('CPython', '2.4.3', '', '', 'truncation', 'd', 'GCC')),
            ('2.4.3 (truncation, ) \n[GCC]',
             ('CPython', '2.4.3', '', '', 'truncation', '', 'GCC')),
            ('2.4.3 (truncation,) \n[GCC]',
             ('CPython', '2.4.3', '', '', 'truncation', '', 'GCC')),
            ('2.4.3 (truncation) \n[GCC]',
             ('CPython', '2.4.3', '', '', 'truncation', '', 'GCC')),
            ):
            # branch and revision are not "parsed", but fetched
            # from sys._git.  Ignore them
            (name, version, branch, revision, buildno, builddate, compiler) \
                   = platform._sys_version(input)
            self.assertEqual(
                (name, version, '', '', buildno, builddate, compiler), output)

        # Tests for python_implementation(), python_version(), python_branch(),
        # python_revision(), python_build(), and python_compiler().
        sys_versions = {
            ("2.6.1 (r261:67515, Dec  6 2008, 15:26:00) \n[GCC 4.0.1 (Apple Computer, Inc. build 5370)]",
             ('CPython', 'tags/r261', '67515'), self.save_platform)
            :
                ("CPython", "2.6.1", "tags/r261", "67515",
                 ('r261:67515', 'Dec  6 2008 15:26:00'),
                 'GCC 4.0.1 (Apple Computer, Inc. build 5370)'),

            ("IronPython 2.0 (2.0.0.0) on .NET 2.0.50727.3053", None, "cli")
            :
                ("IronPython", "2.0.0", "", "", ("", ""),
                 ".NET 2.0.50727.3053"),

            ("2.6.1 (IronPython 2.6.1 (2.6.10920.0) on .NET 2.0.50727.1433)", None, "cli")
            :
                ("IronPython", "2.6.1", "", "", ("", ""),
                 ".NET 2.0.50727.1433"),

            ("2.7.4 (IronPython 2.7.4 (2.7.0.40) on Mono 4.0.30319.1 (32-bit))", None, "cli")
            :
                ("IronPython", "2.7.4", "", "", ("", ""),
                 "Mono 4.0.30319.1 (32-bit)"),

            ("2.5 (trunk:6107, Mar 26 2009, 13:02:18) \n[Java HotSpot(TM) Client VM (\"Apple Computer, Inc.\")]",
            ('Jython', 'trunk', '6107'), "java1.5.0_16")
            :
                ("Jython", "2.5.0", "trunk", "6107",
                 ('trunk:6107', 'Mar 26 2009'), "java1.5.0_16"),

            ("2.5.2 (63378, Mar 26 2009, 18:03:29)\n[PyPy 1.0.0]",
             ('PyPy', 'trunk', '63378'), self.save_platform)
            :
                ("PyPy", "2.5.2", "trunk", "63378", ('63378', 'Mar 26 2009'),
                 "")
            }
        for (version_tag, scm, sys_platform), info in \
                sys_versions.items():
            sys.version = version_tag
            if scm is None:
                if hasattr(sys, "_git"):
                    del sys._git
            else:
                sys._git = scm
            if sys_platform is not None:
                sys.platform = sys_platform
            self.assertEqual(platform.python_implementation(), info[0])
            self.assertEqual(platform.python_version(), info[1])
            self.assertEqual(platform.python_branch(), info[2])
            self.assertEqual(platform.python_revision(), info[3])
            self.assertEqual(platform.python_build(), info[4])
            self.assertEqual(platform.python_compiler(), info[5])

    def test_system_alias(self):
        res = platform.system_alias(
            platform.system(),
            platform.release(),
            platform.version(),
        )

    def test_uname(self):
        res = platform.uname()
        self.assertTrue(any(res))
        self.assertEqual(res[0], res.system)
        self.assertEqual(res[1], res.node)
        self.assertEqual(res[2], res.release)
        self.assertEqual(res[3], res.version)
        self.assertEqual(res[4], res.machine)
        self.assertEqual(res[5], res.processor)

    @unittest.skipUnless(sys.platform.startswith('win'), "windows only test")
    def test_uname_win32_ARCHITEW6432(self):
        # Issue 7860: make sure we get architecture from the correct variable
        # on 64 bit Windows: if PROCESSOR_ARCHITEW6432 exists we should be
        # using it, per
        # http://blogs.msdn.com/david.wang/archive/2006/03/26/HOWTO-Detect-Process-Bitness.aspx
        try:
            with support.EnvironmentVarGuard() as environ:
                if 'PROCESSOR_ARCHITEW6432' in environ:
                    del environ['PROCESSOR_ARCHITEW6432']
                environ['PROCESSOR_ARCHITECTURE'] = 'foo'
                platform._uname_cache = None
                system, node, release, version, machine, processor = platform.uname()
                self.assertEqual(machine, 'foo')
                environ['PROCESSOR_ARCHITEW6432'] = 'bar'
                platform._uname_cache = None
                system, node, release, version, machine, processor = platform.uname()
                self.assertEqual(machine, 'bar')
        finally:
            platform._uname_cache = None

    def test_java_ver(self):
        res = platform.java_ver()
        if sys.platform == 'java':
            self.assertTrue(all(res))

    def test_win32_ver(self):
        res = platform.win32_ver()

    def test_mac_ver(self):
        res = platform.mac_ver()

        if platform.uname().system == 'Darwin':
            # We're on a MacOSX system, check that
            # the right version information is returned
            fd = os.popen('sw_vers', 'r')
            real_ver = None
            for ln in fd:
                if ln.startswith('ProductVersion:'):
                    real_ver = ln.strip().split()[-1]
                    break
            fd.close()
            self.assertFalse(real_ver is None)
            result_list = res[0].split('.')
            # macOS 11.0 (Big Sur) may report its version number
            # as 10.16 if the executable is built with an older
            # SDK target but sw_vers reports 11.0.
            if result_list == ['10', '16']:
                result_list = ['11', '0']
            expect_list = real_ver.split('.')
            len_diff = len(result_list) - len(expect_list)
            # On Snow Leopard, sw_vers reports 10.6.0 as 10.6
            if len_diff > 0:
                expect_list.extend(['0'] * len_diff)
            self.assertEqual(result_list, expect_list)

            # res[1] claims to contain
            # (version, dev_stage, non_release_version)
            # That information is no longer available
            self.assertEqual(res[1], ('', '', ''))

            if sys.byteorder == 'little':
                self.assertIn(res[2], ('i386', 'x86_64'))
            else:
                self.assertEqual(res[2], 'PowerPC')


    @unittest.skipUnless(sys.platform == 'darwin', "OSX only test")
    def test_mac_ver_with_fork(self):
        # Issue7895: platform.mac_ver() crashes when using fork without exec
        #
        # This test checks that the fix for that issue works.
        #
        pid = os.fork()
        if pid == 0:
            # child
            info = platform.mac_ver()
            os._exit(0)

        else:
            # parent
            cpid, sts = os.waitpid(pid, 0)
            self.assertEqual(cpid, pid)
            self.assertEqual(sts, 0)

    def test_dist(self):
        with warnings.catch_warnings():
            warnings.filterwarnings(
                'ignore',
                r'dist\(\) and linux_distribution\(\) '
                'functions are deprecated .*',
                DeprecationWarning,
            )
            res = platform.dist()

    def test_libc_ver(self):
        if os.path.isdir(sys.executable) and \
           os.path.exists(sys.executable+'.exe'):
            # Cygwin horror
            executable = sys.executable + '.exe'
        elif sys.platform == "win32" and not os.path.exists(sys.executable):
            # App symlink appears to not exist, but we want the
            # real executable here anyway
            import _winapi
            executable = _winapi.GetModuleFileName(0)
        else:
            executable = sys.executable
        res = platform.libc_ver(executable)

        self.addCleanup(support.unlink, support.TESTFN)
        with open(support.TESTFN, 'wb') as f:
            f.write(b'x'*(16384-10))
            f.write(b'GLIBC_1.23.4\0GLIBC_1.9\0GLIBC_1.21\0')
        self.assertEqual(platform.libc_ver(support.TESTFN),
                         ('glibc', '1.23.4'))

    @support.cpython_only
    def test__comparable_version(self):
        from platform import _comparable_version as V
        self.assertEqual(V('1.2.3'), V('1.2.3'))
        self.assertLess(V('1.2.3'), V('1.2.10'))
        self.assertEqual(V('1.2.3.4'), V('1_2-3+4'))
        self.assertLess(V('1.2spam'), V('1.2dev'))
        self.assertLess(V('1.2dev'), V('1.2alpha'))
        self.assertLess(V('1.2dev'), V('1.2a'))
        self.assertLess(V('1.2alpha'), V('1.2beta'))
        self.assertLess(V('1.2a'), V('1.2b'))
        self.assertLess(V('1.2beta'), V('1.2c'))
        self.assertLess(V('1.2b'), V('1.2c'))
        self.assertLess(V('1.2c'), V('1.2RC'))
        self.assertLess(V('1.2c'), V('1.2rc'))
        self.assertLess(V('1.2RC'), V('1.2.0'))
        self.assertLess(V('1.2rc'), V('1.2.0'))
        self.assertLess(V('1.2.0'), V('1.2pl'))
        self.assertLess(V('1.2.0'), V('1.2p'))

        self.assertLess(V('1.5.1'), V('1.5.2b2'))
        self.assertLess(V('3.10a'), V('161'))
        self.assertEqual(V('8.02'), V('8.02'))
        self.assertLess(V('3.4j'), V('1996.07.12'))
        self.assertLess(V('3.1.1.6'), V('3.2.pl0'))
        self.assertLess(V('2g6'), V('11g'))
        self.assertLess(V('0.9'), V('2.2'))
        self.assertLess(V('1.2'), V('1.2.1'))
        self.assertLess(V('1.1'), V('1.2.2'))
        self.assertLess(V('1.1'), V('1.2'))
        self.assertLess(V('1.2.1'), V('1.2.2'))
        self.assertLess(V('1.2'), V('1.2.2'))
        self.assertLess(V('0.4'), V('0.4.0'))
        self.assertLess(V('1.13++'), V('5.5.kw'))
        self.assertLess(V('0.960923'), V('2.2beta29'))

    def test_parse_release_file(self):

        for input, output in (
            # Examples of release file contents:
            ('SuSE Linux 9.3 (x86-64)', ('SuSE Linux ', '9.3', 'x86-64')),
            ('SUSE LINUX 10.1 (X86-64)', ('SUSE LINUX ', '10.1', 'X86-64')),
            ('SUSE LINUX 10.1 (i586)', ('SUSE LINUX ', '10.1', 'i586')),
            ('Fedora Core release 5 (Bordeaux)', ('Fedora Core', '5', 'Bordeaux')),
            ('Red Hat Linux release 8.0 (Psyche)', ('Red Hat Linux', '8.0', 'Psyche')),
            ('Red Hat Linux release 9 (Shrike)', ('Red Hat Linux', '9', 'Shrike')),
            ('Red Hat Enterprise Linux release 4 (Nahant)', ('Red Hat Enterprise Linux', '4', 'Nahant')),
            ('CentOS release 4', ('CentOS', '4', None)),
            ('Rocks release 4.2.1 (Cydonia)', ('Rocks', '4.2.1', 'Cydonia')),
            ('', ('', '', '')), # If there's nothing there.
            ):
            self.assertEqual(platform._parse_release_file(input), output)

    def test_popen(self):
        mswindows = (sys.platform == "win32")

        if mswindows:
            command = '"{}" -c "print(\'Hello\')"'.format(sys.executable)
        else:
            command = "'{}' -c 'print(\"Hello\")'".format(sys.executable)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            with platform.popen(command) as stdout:
                hello = stdout.read().strip()
                stdout.close()
                self.assertEqual(hello, "Hello")

        data = 'plop'
        if mswindows:
            command = '"{}" -c "import sys; data=sys.stdin.read(); exit(len(data))"'
        else:
            command = "'{}' -c 'import sys; data=sys.stdin.read(); exit(len(data))'"
        command = command.format(sys.executable)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            with platform.popen(command, 'w') as stdin:
                stdout = stdin.write(data)
                ret = stdin.close()
                self.assertIsNotNone(ret)
                if os.name == 'nt':
                    returncode = ret
                else:
                    returncode = ret >> 8
                self.assertEqual(returncode, len(data))

    def test_linux_distribution_encoding(self):
        # Issue #17429
        with tempfile.TemporaryDirectory() as tempdir:
            filename = os.path.join(tempdir, 'fedora-release')
            with open(filename, 'w', encoding='utf-8') as f:
                f.write('Fedora release 19 (Schr\xf6dinger\u2019s Cat)\n')

            with mock.patch('platform._UNIXCONFDIR', tempdir):
                with warnings.catch_warnings():
                    warnings.filterwarnings(
                        'ignore',
                        r'dist\(\) and linux_distribution\(\) '
                        'functions are deprecated .*',
                        DeprecationWarning,
                    )
                    distname, version, distid = platform.linux_distribution()

                self.assertEqual(distname, 'Fedora')
            self.assertEqual(version, '19')
            self.assertEqual(distid, 'Schr\xf6dinger\u2019s Cat')


class DeprecationTest(unittest.TestCase):

    def test_dist_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            platform.dist()
        self.assertEqual(str(cm.warning),
                         'dist() and linux_distribution() functions are '
                         'deprecated in Python 3.5')

    def test_linux_distribution_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            platform.linux_distribution()
        self.assertEqual(str(cm.warning),
                         'dist() and linux_distribution() functions are '
                         'deprecated in Python 3.5')

if __name__ == '__main__':
    unittest.main()
