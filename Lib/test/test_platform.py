import contextlib
import copy
import io
import itertools
import os
import pickle
import platform
import subprocess
import sys
import unittest
from unittest import mock

from test import support
from test.support import os_helper

try:
    # Some of the iOS tests need ctypes to operate.
    # Confirm that the ctypes module is available
    # is available.
    import _ctypes
except ImportError:
    _ctypes = None

FEDORA_OS_RELEASE = """\
NAME=Fedora
VERSION="32 (Thirty Two)"
ID=fedora
VERSION_ID=32
VERSION_CODENAME=""
PLATFORM_ID="platform:f32"
PRETTY_NAME="Fedora 32 (Thirty Two)"
ANSI_COLOR="0;34"
LOGO=fedora-logo-icon
CPE_NAME="cpe:/o:fedoraproject:fedora:32"
HOME_URL="https://fedoraproject.org/"
DOCUMENTATION_URL="https://docs.fedoraproject.org/en-US/fedora/f32/system-administrators-guide/"
SUPPORT_URL="https://fedoraproject.org/wiki/Communicating_and_getting_help"
BUG_REPORT_URL="https://bugzilla.redhat.com/"
REDHAT_BUGZILLA_PRODUCT="Fedora"
REDHAT_BUGZILLA_PRODUCT_VERSION=32
REDHAT_SUPPORT_PRODUCT="Fedora"
REDHAT_SUPPORT_PRODUCT_VERSION=32
PRIVACY_POLICY_URL="https://fedoraproject.org/wiki/Legal:PrivacyPolicy"
"""

UBUNTU_OS_RELEASE = """\
NAME="Ubuntu"
VERSION="20.04.1 LTS (Focal Fossa)"
ID=ubuntu
ID_LIKE=debian
PRETTY_NAME="Ubuntu 20.04.1 LTS"
VERSION_ID="20.04"
HOME_URL="https://www.ubuntu.com/"
SUPPORT_URL="https://help.ubuntu.com/"
BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
VERSION_CODENAME=focal
UBUNTU_CODENAME=focal
"""

TEST_OS_RELEASE = r"""
# test data
ID_LIKE="egg spam viking"
EMPTY=
# comments and empty lines are ignored

SINGLE_QUOTE='single'
EMPTY_SINGLE=''
DOUBLE_QUOTE="double"
EMPTY_DOUBLE=""
QUOTES="double\'s"
SPECIALS="\$\`\\\'\""
# invalid lines
=invalid
=
INVALID
IN-VALID=value
IN VALID=value
"""


class PlatformTest(unittest.TestCase):
    def clear_caches(self):
        platform._platform_cache.clear()
        platform._sys_version_cache.clear()
        platform._uname_cache = None
        platform._os_release_cache = None

    def test_invalidate_caches(self):
        self.clear_caches()

        self.assertDictEqual(platform._platform_cache, {})
        self.assertDictEqual(platform._sys_version_cache, {})
        self.assertIsNone(platform._uname_cache)
        self.assertIsNone(platform._os_release_cache)

        # fill the cached entries (some have side effects on others)
        platform.platform()                 # for platform._platform_cache
        platform.python_implementation()    # for platform._sys_version_cache
        platform.uname()                    # for platform._uname_cache

        # check that the cache are filled
        self.assertNotEqual(platform._platform_cache, {})
        self.assertNotEqual(platform._sys_version_cache, {})
        self.assertIsNotNone(platform._uname_cache)

        try:
            platform.freedesktop_os_release()
        except OSError:
            self.assertIsNone(platform._os_release_cache)
        else:
            self.assertIsNotNone(platform._os_release_cache)

        with self.subTest('clear platform caches'):
            platform.invalidate_caches()
            self.assertDictEqual(platform._platform_cache, {})
            self.assertDictEqual(platform._sys_version_cache, {})
            self.assertIsNone(platform._uname_cache)
            self.assertIsNone(platform._os_release_cache)

    def test_architecture(self):
        res = platform.architecture()

    @os_helper.skip_unless_symlink
    @support.requires_subprocess()
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

            ("3.10.8 (tags/v3.10.8:aaaf517424, Feb 14 2023, 16:28:12) [GCC 9.4.0]",
             None, "linux")
            :
                ('CPython', '3.10.8', '', '',
                ('tags/v3.10.8:aaaf517424', 'Feb 14 2023 16:28:12'), 'GCC 9.4.0'),

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

        with self.assertRaises(ValueError):
            platform._sys_version('2. 4.3 (truncation) \n[GCC]')

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

        if os.name == "posix":
            uname = os.uname()
            self.assertEqual(res.node, uname.nodename)
            self.assertEqual(res.version, uname.version)
            self.assertEqual(res.machine, uname.machine)

            if sys.platform == "android":
                self.assertEqual(res.system, "Android")
                self.assertEqual(res.release, platform.android_ver().release)
            elif sys.platform == "ios":
                # Platform module needs ctypes for full operation. If ctypes
                # isn't available, there's no ObjC module, and dummy values are
                # returned.
                if _ctypes:
                    self.assertIn(res.system, {"iOS", "iPadOS"})
                    self.assertEqual(res.release, platform.ios_ver().release)
                else:
                    self.assertEqual(res.system, "")
                    self.assertEqual(res.release, "")
            else:
                self.assertEqual(res.system, uname.sysname)
                self.assertEqual(res.release, uname.release)


    @unittest.skipUnless(sys.platform.startswith('win'), "windows only test")
    def test_uname_win32_without_wmi(self):
        def raises_oserror(*a):
            raise OSError()

        with support.swap_attr(platform, '_wmi_query', raises_oserror):
            self.test_uname()

    def test_uname_cast_to_tuple(self):
        res = platform.uname()
        expected = (
            res.system, res.node, res.release, res.version, res.machine,
            res.processor,
        )
        self.assertEqual(tuple(res), expected)

    def test_uname_replace(self):
        res = platform.uname()
        new = res._replace(
            system='system', node='node', release='release',
            version='version', machine='machine')
        self.assertEqual(new.system, 'system')
        self.assertEqual(new.node, 'node')
        self.assertEqual(new.release, 'release')
        self.assertEqual(new.version, 'version')
        self.assertEqual(new.machine, 'machine')
        # processor cannot be replaced
        self.assertEqual(new.processor, res.processor)

    def test_uname_copy(self):
        uname = platform.uname()
        self.assertEqual(copy.copy(uname), uname)
        self.assertEqual(copy.deepcopy(uname), uname)

    def test_uname_pickle(self):
        orig = platform.uname()
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(protocol=proto):
                pickled = pickle.dumps(orig, proto)
                restored = pickle.loads(pickled)
                self.assertEqual(restored, orig)

    def test_uname_slices(self):
        res = platform.uname()
        expected = tuple(res)
        self.assertEqual(res[:], expected)
        self.assertEqual(res[:5], expected[:5])

    def test_uname_fields(self):
        self.assertIn('processor', platform.uname()._fields)

    def test_uname_asdict(self):
        res = platform.uname()._asdict()
        self.assertEqual(len(res), 6)
        self.assertIn('processor', res)

    @unittest.skipIf(sys.platform in ['win32', 'OpenVMS'], "uname -p not used")
    @support.requires_subprocess()
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

        # We also need to suppress WMI checks, as those are reliable and
        # overrule the environment variables
        def raises_oserror(*a):
            raise OSError()

        with support.swap_attr(platform, '_wmi_query', raises_oserror):
            with os_helper.EnvironmentVarGuard() as environ:
                try:
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

    @unittest.skipUnless(support.MS_WINDOWS, 'This test only makes sense on Windows')
    def test_win32_ver(self):
        release1, version1, csd1, ptype1 = 'a', 'b', 'c', 'd'
        res = platform.win32_ver(release1, version1, csd1, ptype1)
        self.assertEqual(len(res), 4)
        release, version, csd, ptype = res
        if release:
            # Currently, release names always come from internal dicts,
            # but this could change over time. For now, we just check that
            # release is something different from what we have passed.
            self.assertNotEqual(release, release1)
        if version:
            # It is rather hard to test explicit version without
            # going deep into the details.
            self.assertIn('.', version)
            for v in version.split('.'):
                int(v)  # should not fail
        if csd:
            self.assertTrue(csd.startswith('SP'), msg=csd)
        if ptype:
            if os.cpu_count() > 1:
                self.assertIn('Multiprocessor', ptype)
            else:
                self.assertIn('Uniprocessor', ptype)

    @unittest.skipIf(support.MS_WINDOWS, 'This test only makes sense on non Windows')
    def test_win32_ver_on_non_windows(self):
        release, version, csd, ptype = 'a', '1.0', 'c', 'd'
        res = platform.win32_ver(release, version, csd, ptype)
        self.assertSequenceEqual(res, (release, version, csd, ptype), seq_type=tuple)

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
            # For compatibility with older binaries, macOS 11.x may report
            # itself as '10.16' rather than '11.x.y'.
            if result_list != ['10', '16']:
                self.assertEqual(result_list, expect_list)

            # res[1] claims to contain
            # (version, dev_stage, non_release_version)
            # That information is no longer available
            self.assertEqual(res[1], ('', '', ''))

            if sys.byteorder == 'little':
                self.assertIn(res[2], ('i386', 'x86_64', 'arm64'))
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

    def test_ios_ver(self):
        result = platform.ios_ver()

        # ios_ver is only fully available on iOS where ctypes is available.
        if sys.platform == "ios" and _ctypes:
            system, release, model, is_simulator = result
            # Result is a namedtuple
            self.assertEqual(result.system, system)
            self.assertEqual(result.release, release)
            self.assertEqual(result.model, model)
            self.assertEqual(result.is_simulator, is_simulator)

            # We can't assert specific values without reproducing the logic of
            # ios_ver(), so we check that the values are broadly what we expect.

            # System is either iOS or iPadOS, depending on the test device
            self.assertIn(system, {"iOS", "iPadOS"})

            # Release is a numeric version specifier with at least 2 parts
            parts = release.split(".")
            self.assertGreaterEqual(len(parts), 2)
            self.assertTrue(all(part.isdigit() for part in parts))

            # If this is a simulator, we get a high level device descriptor
            # with no identifying model number. If this is a physical device,
            # we get a model descriptor like "iPhone13,1"
            if is_simulator:
                self.assertIn(model, {"iPhone", "iPad"})
            else:
                self.assertTrue(
                    (model.startswith("iPhone") or model.startswith("iPad"))
                    and "," in model
                )

            self.assertEqual(type(is_simulator), bool)
        else:
            # On non-iOS platforms, calling ios_ver doesn't fail; you get
            # default values
            self.assertEqual(result.system, "")
            self.assertEqual(result.release, "")
            self.assertEqual(result.model, "")
            self.assertFalse(result.is_simulator)

            # Check the fallback values can be overridden by arguments
            override = platform.ios_ver("Foo", "Bar", "Whiz", True)
            self.assertEqual(override.system, "Foo")
            self.assertEqual(override.release, "Bar")
            self.assertEqual(override.model, "Whiz")
            self.assertTrue(override.is_simulator)

    @unittest.skipIf(support.is_emscripten, "Does not apply to Emscripten")
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

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        with mock.patch('os.confstr', create=True, return_value='mock 1.0'):
            # test os.confstr() code path
            self.assertEqual(platform.libc_ver(), ('mock', '1.0'))

            # test the different regular expressions
            for data, expected in (
                (b'__libc_init', ('libc', '')),
                (b'GLIBC_2.9', ('glibc', '2.9')),
                (b'libc.so.1.2.5', ('libc', '1.2.5')),
                (b'libc_pthread.so.1.2.5', ('libc', '1.2.5_pthread')),
                (b'/aports/main/musl/src/musl-1.2.5', ('musl', '1.2.5')),
                # musl uses semver, but we accept some variations anyway:
                (b'/aports/main/musl/src/musl-12.5', ('musl', '12.5')),
                (b'/aports/main/musl/src/musl-1.2.5.7', ('musl', '1.2.5.7')),
                (b'', ('', '')),
            ):
                with open(filename, 'wb') as fp:
                    fp.write(b'[xxx%sxxx]' % data)
                    fp.flush()

                # os.confstr() must not be used if executable is set
                self.assertEqual(platform.libc_ver(executable=filename),
                                 expected)

        # binary containing multiple versions: get the most recent,
        # make sure that eg 1.9 is seen as older than 1.23.4, and that
        # the arguments don't count even if they are set.
        chunksize = 200
        for data, expected in (
                (b'GLIBC_1.23.4\0GLIBC_1.9\0GLIBC_1.21\0', ('glibc', '1.23.4')),
                (b'libc.so.2.4\0libc.so.9\0libc.so.23.1\0', ('libc', '23.1')),
                (b'musl-1.4.1\0musl-2.1.1\0musl-2.0.1\0', ('musl', '2.1.1')),
                (b'no match here, so defaults are used', ('test', '100.1.0')),
            ):
            with open(filename, 'wb') as f:
                # test match at chunk boundary
                f.write(b'x'*(chunksize - 10))
                f.write(data)
            self.assertEqual(
                expected,
                platform.libc_ver(
                    filename,
                    lib='test',
                    version='100.1.0',
                    chunksize=chunksize,
                    ),
                )


    def test_android_ver(self):
        res = platform.android_ver()
        self.assertIsInstance(res, tuple)
        self.assertEqual(res, (res.release, res.api_level, res.manufacturer,
                               res.model, res.device, res.is_emulator))

        if sys.platform == "android":
            for name in ["release", "manufacturer", "model", "device"]:
                with self.subTest(name):
                    value = getattr(res, name)
                    self.assertIsInstance(value, str)
                    self.assertNotEqual(value, "")

            self.assertIsInstance(res.api_level, int)
            self.assertGreaterEqual(res.api_level, sys.getandroidapilevel())

            self.assertIsInstance(res.is_emulator, bool)

        # When not running on Android, it should return the default values.
        else:
            self.assertEqual(res.release, "")
            self.assertEqual(res.api_level, 0)
            self.assertEqual(res.manufacturer, "")
            self.assertEqual(res.model, "")
            self.assertEqual(res.device, "")
            self.assertEqual(res.is_emulator, False)

            # Default values may also be overridden using parameters.
            res = platform.android_ver(
                "alpha", 1, "bravo", "charlie", "delta", True)
            self.assertEqual(res.release, "alpha")
            self.assertEqual(res.api_level, 1)
            self.assertEqual(res.manufacturer, "bravo")
            self.assertEqual(res.model, "charlie")
            self.assertEqual(res.device, "delta")
            self.assertEqual(res.is_emulator, True)

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
        with mock.patch.object(sys, "platform", "darwin"), \
             mock.patch.object(platform, 'uname', return_value=uname), \
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

    def test_freedesktop_os_release(self):
        self.addCleanup(self.clear_caches)
        self.clear_caches()

        if any(os.path.isfile(fn) for fn in platform._os_release_candidates):
            info = platform.freedesktop_os_release()
            self.assertIn("NAME", info)
            self.assertIn("ID", info)

            info["CPYTHON_TEST"] = "test"
            self.assertNotIn(
                "CPYTHON_TEST",
                platform.freedesktop_os_release()
            )
        else:
            with self.assertRaises(OSError):
                platform.freedesktop_os_release()

    def test_parse_os_release(self):
        info = platform._parse_os_release(FEDORA_OS_RELEASE.splitlines())
        self.assertEqual(info["NAME"], "Fedora")
        self.assertEqual(info["ID"], "fedora")
        self.assertNotIn("ID_LIKE", info)
        self.assertEqual(info["VERSION_CODENAME"], "")

        info = platform._parse_os_release(UBUNTU_OS_RELEASE.splitlines())
        self.assertEqual(info["NAME"], "Ubuntu")
        self.assertEqual(info["ID"], "ubuntu")
        self.assertEqual(info["ID_LIKE"], "debian")
        self.assertEqual(info["VERSION_CODENAME"], "focal")

        info = platform._parse_os_release(TEST_OS_RELEASE.splitlines())
        expected = {
            "ID": "linux",
            "NAME": "Linux",
            "PRETTY_NAME": "Linux",
            "ID_LIKE": "egg spam viking",
            "EMPTY": "",
            "DOUBLE_QUOTE": "double",
            "EMPTY_DOUBLE": "",
            "SINGLE_QUOTE": "single",
            "EMPTY_SINGLE": "",
            "QUOTES": "double's",
            "SPECIALS": "$`\\'\"",
        }
        self.assertEqual(info, expected)
        self.assertEqual(len(info["SPECIALS"]), 5)


class CommandLineTest(unittest.TestCase):
    def setUp(self):
        platform.invalidate_caches()
        self.addCleanup(platform.invalidate_caches)

    def invoke_platform(self, *flags):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            platform._main(args=flags)
        return output.getvalue()

    def test_unknown_flag(self):
        with self.assertRaises(SystemExit):
            output = io.StringIO()
            # suppress argparse error message
            with contextlib.redirect_stderr(output):
                _ = self.invoke_platform('--unknown')
            self.assertStartsWith(output, "usage: ")

    def test_invocation(self):
        flags = (
            "--terse", "--nonaliased", "terse", "nonaliased"
        )

        for r in range(len(flags) + 1):
            for combination in itertools.combinations(flags, r):
                self.invoke_platform(*combination)

    def test_arg_parsing(self):
        # For backwards compatibility, the `aliased` and `terse` parameters are
        # computed based on a combination of positional arguments and flags.
        #
        # Test that the arguments are correctly passed to the underlying
        # `platform.platform()` call.
        options = (
            (["--nonaliased"], False, False),
            (["nonaliased"], False, False),
            (["--terse"], True, True),
            (["terse"], True, True),
            (["nonaliased", "terse"], False, True),
            (["--nonaliased", "terse"], False, True),
            (["--terse", "nonaliased"], False, True),
        )

        for flags, aliased, terse in options:
            with self.subTest(flags=flags, aliased=aliased, terse=terse):
                with mock.patch.object(platform, 'platform') as obj:
                    self.invoke_platform(*flags)
                    obj.assert_called_once_with(aliased, terse)

    @support.force_not_colorized
    def test_help(self):
        output = io.StringIO()

        with self.assertRaises(SystemExit):
            with contextlib.redirect_stdout(output):
                platform._main(args=["--help"])

        self.assertStartsWith(output.getvalue(), "usage:")


if __name__ == '__main__':
    unittest.main()
