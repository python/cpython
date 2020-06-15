import os
import platform
import subprocess
import sys
import unittest
from unittest import mock

from test import support


class PlatformTest(unittest.TestCase):
    def clear_caches(self):
        platform._platform_cache.clear()
        platform._sys_version_cache.clear()
        platform._uname_cache = None

    def test_architecture(self):
        res = platform.architecture()

    @support.skip_unless_symlink
    def test_architecture_via_symlink(self): # issue3762
        with support.PythonSymlink() as py:
            cmd = "-c", "import platform; print(platform.architecture())"
            self.assertEqual(py.call_real(*cmd), py.call_link(*cmd))

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
        self.assertEqual(res[-6], res.system)
        self.assertEqual(res[1], res.node)
        self.assertEqual(res[-5], res.node)
        self.assertEqual(res[2], res.release)
        self.assertEqual(res[-4], res.release)
        self.assertEqual(res[3], res.version)
        self.assertEqual(res[-3], res.version)
        self.assertEqual(res[4], res.machine)
        self.assertEqual(res[-2], res.machine)
        self.assertEqual(res[5], res.processor)
        self.assertEqual(res[-1], res.processor)
        self.assertEqual(len(res), 6)

    def test_uname_cast_to_tuple(self):
        res = platform.uname()
        expected = (
            res.system, res.node, res.release, res.version, res.machine,
            res.processor,
        )
        self.assertEqual(tuple(res), expected)

    @unittest.skipIf(sys.platform in ['win32', 'OpenVMS'], "uname -p not used")
    def test_uname_processor(self):
        """
        On some systems, the processor must match the output
        of 'uname -p'. See Issue 35967 for rationale.
        """
        try:
            proc_res = subprocess.check_output(['uname', '-p'], text=True).strip()
            expect = platform._unknown_as_blank(proc_res)
        except (OSError, subprocess.CalledProcessError):
            expect = ''
        self.assertEqual(platform.uname().processor, expect)

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
            # We are on a macOS system, check that the right version
            # information is returned
            output = subprocess.check_output(['sw_vers'], text=True)
            for line in output.splitlines():
                if line.startswith('ProductVersion:'):
                    real_ver = line.strip().split()[-1]
                    break
            else:
                self.fail(f"failed to parse sw_vers output: {output!r}")

            result_list = res[0].split('.')
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
            support.wait_process(pid, exitcode=0)

    def test_libc_ver(self):
        # check that libc_ver(executable) doesn't raise an exception
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
        platform.libc_ver(executable)

        filename = support.TESTFN
        self.addCleanup(support.unlink, filename)

        with mock.patch('os.confstr', create=True, return_value='mock 1.0'):
            # test os.confstr() code path
            self.assertEqual(platform.libc_ver(), ('mock', '1.0'))

            # test the different regular expressions
            for data, expected in (
                (b'__libc_init', ('libc', '')),
                (b'GLIBC_2.9', ('glibc', '2.9')),
                (b'libc.so.1.2.5', ('libc', '1.2.5')),
                (b'libc_pthread.so.1.2.5', ('libc', '1.2.5_pthread')),
                (b'', ('', '')),
            ):
                with open(filename, 'wb') as fp:
                    fp.write(b'[xxx%sxxx]' % data)
                    fp.flush()

                # os.confstr() must not be used if executable is set
                self.assertEqual(platform.libc_ver(executable=filename),
                                 expected)

        # binary containing multiple versions: get the most recent,
        # make sure that 1.9 is seen as older than 1.23.4
        chunksize = 16384
        with open(filename, 'wb') as f:
            # test match at chunk boundary
            f.write(b'x'*(chunksize - 10))
            f.write(b'GLIBC_1.23.4\0GLIBC_1.9\0GLIBC_1.21\0')
        self.assertEqual(platform.libc_ver(filename, chunksize=chunksize),
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


    def test_macos(self):
        self.addCleanup(self.clear_caches)

        uname = ('Darwin', 'hostname', '17.7.0',
                 ('Darwin Kernel Version 17.7.0: '
                  'Thu Jun 21 22:53:14 PDT 2018; '
                  'root:xnu-4570.71.2~1/RELEASE_X86_64'),
                 'x86_64', 'i386')
        arch = ('64bit', '')
        with mock.patch.object(platform, 'uname', return_value=uname), \
             mock.patch.object(platform, 'architecture', return_value=arch):
            for mac_ver, expected_terse, expected in [
                # darwin: mac_ver() returns empty strings
                (('', '', ''),
                 'Darwin-17.7.0',
                 'Darwin-17.7.0-x86_64-i386-64bit'),
                # macOS: mac_ver() returns macOS version
                (('10.13.6', ('', '', ''), 'x86_64'),
                 'macOS-10.13.6',
                 'macOS-10.13.6-x86_64-i386-64bit'),
            ]:
                with mock.patch.object(platform, 'mac_ver',
                                       return_value=mac_ver):
                    self.clear_caches()
                    self.assertEqual(platform.platform(terse=1), expected_terse)
                    self.assertEqual(platform.platform(), expected)


if __name__ == '__main__':
    unittest.main()
