
import pathlib
import shlex
import sys
import sysconfig
import tempfile
import test.support
import unittest

import test.support.script_helper


_CPYTHON = pathlib.Path(test.support.REPO_ROOT).resolve()
_TOOLS_JIT = _CPYTHON / "Tools" / "jit"
_TOOLS_JIT_TEST = _TOOLS_JIT / "test"
_TOOLS_JIT_BUILD_PY = _TOOLS_JIT / "build.py"

@unittest.skipIf(test.support.Py_DEBUG, "XXX")
@unittest.skipUnless(sys._jit.is_available(), "XXX")
@unittest.skipIf(test.support.Py_GIL_DISABLED, "XXX")
@unittest.skipUnless(sysconfig.is_python_build(), "XXX")
class TestJITStencils(unittest.TestCase):

    def test_jit_stencils(self):
        self.maxDiff = None
        found = False
        pyconfig_dir = pathlib.Path(sysconfig.get_config_h_filename()).parent
        with tempfile.TemporaryDirectory() as work:
            output_dir = pathlib.Path(work).resolve()
            for test_jit_stencils_h in sorted(_TOOLS_JIT_TEST.glob("test_jit_stencils-*.h")):
                target = test_jit_stencils_h.stem.removeprefix("test_jit_stencils-")
                jit_stencils_h = output_dir / f"jit_stencils-{target}.h"
                with self.subTest(target):
                    # relative = jit_stencils_h.relative_to(_CPYTHON)
                    result, args = test.support.script_helper.run_python_until_end(
                        _TOOLS_JIT_BUILD_PY,
                        "--input-file", _TOOLS_JIT_TEST / "test_executor_cases.c.h",
                        "--output-dir", output_dir,
                        "--pyconfig-dir", pyconfig_dir,
                        target,
                        __isolated=False
                    )
                    if result.rc:
                        self.skipTest(shlex.join(map(str, args)))
                    found = True
                    expected = test_jit_stencils_h.read_text()
                    actual = "".join(jit_stencils_h.read_text().splitlines(True)[3:])
                    self.assertEqual(expected, actual)
        self.assertTrue(found, "No JIT stencil tests run!")
