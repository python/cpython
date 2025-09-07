import sys
import unittest
import sysconfig

from test.support import subTests
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')


class Test_ABIInfo_Check(unittest.TestCase):
    @subTests('modname', (None, 'test_mod'))
    def test_zero(self, modname):
        _testcapi.pyabiinfo_check(modname, 0, 0, 0, 0, 0)
        _testcapi.pyabiinfo_check(modname, 1, 0, 0, 0, 0)

    def test_large_major_version(self):
        with self.assertRaisesRegex(ImportError,
                                    '^PyABIInfo version too high$'):
            _testcapi.pyabiinfo_check(None, 2, 0, 0, 0, 0)
        with self.assertRaisesRegex(ImportError,
                                    '^test_mod: PyABIInfo version too high$'):
            _testcapi.pyabiinfo_check("test_mod", 2, 0, 0, 0, 0)

    @subTests('modname', (None, 'test_mod'))
    def test_large_minor_version(self, modname):
        _testcapi.pyabiinfo_check(modname, 1, 2, 0, 0, 0)

    @subTests('modname', (None, 'test_mod'))
    @subTests('major', (0, 1))
    @subTests('minor', (0, 1, 9))
    @subTests('build', (0, sys.hexversion))
    def test_positive_regular(self, modname, major, minor, build):
        ver = sys.hexversion
        truncated = ver & 0xffff0000
        filled = truncated | 0x12b8
        maxed = truncated | 0xffff
        for abi_version in (0, ver, truncated, filled, maxed):
            with self.subTest(abi_version=abi_version):
                _testcapi.pyabiinfo_check(modname, major, minor, 0,
                                          build, abi_version)

    @subTests('modname', (None, 'test_mod'))
    @subTests('minor', (0, 1, 9))
    @subTests('build', (0, sys.hexversion))
    @subTests('offset', (+0x00010000, -0x00010000))
    def test_negative_regular(self, modname, minor, build, offset):
        ver = sys.hexversion + offset
        truncated = ver & 0xffff0000
        filled = truncated | 0x12b8
        maxed = truncated | 0xffff
        for abi_version in (ver, truncated, filled, maxed):
            with self.subTest(abi_version=abi_version):
                with self.assertRaisesRegex(
                        ImportError,
                        r'incompatible ABI version \(3\.\d+\)$'):
                    _testcapi.pyabiinfo_check(modname, 1, minor, 0,
                                              build,
                                              abi_version)

    @subTests('modname', (None, 'test_mod'))
    @subTests('major', (0, 1))
    @subTests('minor', (0, 1, 9))
    @subTests('build', (0, sys.hexversion))
    @subTests('abi_version', (
        0,
        0x03020000,
        sys.hexversion,
        sys.hexversion & 0xffff0000,
        sys.hexversion - 0x00010000,
    ))
    def test_positive_stable(self, modname, major, minor, build, abi_version):
        _testcapi.pyabiinfo_check(modname, major, minor,
                                  _testcapi.PyABIInfo_STABLE,
                                  build,
                                  abi_version)

    @subTests('modname', (None, 'test_mod'))
    @subTests('minor', (0, 1, 9))
    @subTests('build', (0, sys.hexversion))
    @subTests('abi_version_and_msg', (
        (1, 'invalid'),
        (3, 'invalid'),
        (0x0301ffff, 'invalid'),
        ((sys.hexversion & 0xffff0000) + 0x00010000, 'incompatible future'),
        (sys.hexversion + 0x00010000, 'incompatible future'),
        (0x04000000, 'incompatible future'),
    ))
    def test_negative_stable(self, modname, minor, build, abi_version_and_msg):
        abi_version, msg = abi_version_and_msg
        with self.assertRaisesRegex(
                ImportError,
                rf'{msg} stable ABI version \(\d+\.\d+\)$'):
            _testcapi.pyabiinfo_check(modname, 1, minor,
                                      _testcapi.PyABIInfo_STABLE,
                                      build,
                                      abi_version)

    @subTests('modname', (None, 'test_mod'))
    @subTests('major', (0, 1))
    @subTests('minor', (0, 1, 9))
    @subTests('build', (0, sys.hexversion))
    @subTests('abi_version', (0, sys.hexversion))
    def test_positive_internal(self, modname, major, minor, build, abi_version):
        _testcapi.pyabiinfo_check(modname, major, minor,
                                  _testcapi.PyABIInfo_INTERNAL,
                                  build,
                                  abi_version)

    @subTests('modname', (None, 'test_mod'))
    @subTests('minor', (0, 1, 9))
    @subTests('build', (0, sys.hexversion))
    @subTests('abi_version', (
        sys.hexversion - 0x00010000,
        sys.hexversion - 1,
        sys.hexversion + 1,
        sys.hexversion + 0x00010000,
    ))
    def test_negative_internal(self, modname, minor, build, abi_version):
        with self.assertRaisesRegex(
                ImportError,
                r'incompatible internal ABI \(0x[\da-f]+ != 0x[\da-f]+\)$'):
            _testcapi.pyabiinfo_check(modname, 1, minor,
                                      _testcapi.PyABIInfo_INTERNAL,
                                      build,
                                      abi_version)

    @subTests('modname', (None, 'test_mod'))
    @subTests('minor', (0, 1, 9))
    @subTests('build', (0, sys.hexversion))
    @subTests('ft_flag', (
        0,
        (_testcapi.PyABIInfo_FREETHREADED
            if sysconfig.get_config_var("Py_GIL_DISABLED")
            else _testcapi.PyABIInfo_GIL),
        _testcapi.PyABIInfo_FREETHREADING_AGNOSTIC,
    ))
    def test_positive_freethreading(self, modname, minor, build, ft_flag):
        self.assertEqual(ft_flag & _testcapi.PyABIInfo_FREETHREADING_AGNOSTIC,
                         ft_flag)
        _testcapi.pyabiinfo_check(modname, 1, minor, ft_flag, build, 0)

    @subTests('modname', (None, 'test_mod'))
    @subTests('minor', (0, 1, 9))
    @subTests('build', (0, sys.hexversion))
    def test_negative_freethreading(self, modname, minor, build):
        if sysconfig.get_config_var("Py_GIL_DISABLED"):
            ft_flag = _testcapi.PyABIInfo_GIL
            msg = "incompatible with free-threaded CPython"
        else:
            ft_flag = _testcapi.PyABIInfo_FREETHREADED
            msg = "only compatible with free-threaded CPython"
        with self.assertRaisesRegex(ImportError, msg):
            _testcapi.pyabiinfo_check(modname, 1, minor, ft_flag, build, 0)
