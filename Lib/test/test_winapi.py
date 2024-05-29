# Test the Windows-only _winapi module

import os
import pathlib
import re
import unittest
from test.support import import_helper, os_helper

_winapi = import_helper.import_module('_winapi', required_on=['win'])

class WinAPITests(unittest.TestCase):
    def test_getlongpathname(self):
        testfn = pathlib.Path(os.getenv("ProgramFiles")).parents[-1] / "PROGRA~1"
        if not os.path.isdir(testfn):
            raise unittest.SkipTest("require x:\\PROGRA~1 to test")

        # pathlib.Path will be rejected - only str is accepted
        with self.assertRaises(TypeError):
            _winapi.GetLongPathName(testfn)

        actual = _winapi.GetLongPathName(os.fsdecode(testfn))

        # Can't assume that PROGRA~1 expands to any particular variation, so
        # ensure it matches any one of them.
        candidates = set(testfn.parent.glob("Progra*"))
        self.assertIn(pathlib.Path(actual), candidates)

    def test_getshortpathname(self):
        testfn = pathlib.Path(os.getenv("ProgramFiles"))
        if not os.path.isdir(testfn):
            raise unittest.SkipTest("require '%ProgramFiles%' to test")

        # pathlib.Path will be rejected - only str is accepted
        with self.assertRaises(TypeError):
            _winapi.GetShortPathName(testfn)

        actual = _winapi.GetShortPathName(os.fsdecode(testfn))

        # Should contain "PROGRA~" but we can't predict the number
        self.assertIsNotNone(re.match(r".\:\\PROGRA~\d", actual.upper()), actual)

    def test_namedpipe(self):
        pipe_name = rf"\\.\pipe\LOCAL\{os_helper.TESTFN}"

        # Pipe does not exist, so this raises
        with self.assertRaises(FileNotFoundError):
            _winapi.WaitNamedPipe(pipe_name, 0)

        pipe = _winapi.CreateNamedPipe(
            pipe_name,
            _winapi.PIPE_ACCESS_DUPLEX,
            8, # 8=PIPE_REJECT_REMOTE_CLIENTS
            2, # two instances available
            32, 32, 0, 0)
        self.addCleanup(_winapi.CloseHandle, pipe)

        # Pipe instance is available, so this passes
        _winapi.WaitNamedPipe(pipe_name, 0)

        with open(pipe_name, 'w+b') as pipe2:
            # No instances available, so this times out
            # (WinError 121 does not get mapped to TimeoutError)
            with self.assertRaises(OSError):
                _winapi.WaitNamedPipe(pipe_name, 0)

            _winapi.WriteFile(pipe, b'testdata')
            self.assertEqual(b'testdata', pipe2.read(8))

            self.assertEqual((b'', 0), _winapi.PeekNamedPipe(pipe, 8)[:2])
            pipe2.write(b'testdata')
            pipe2.flush()
            self.assertEqual((b'testdata', 8), _winapi.PeekNamedPipe(pipe, 8)[:2])
