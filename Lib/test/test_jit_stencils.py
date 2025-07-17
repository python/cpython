import asyncio
import pathlib
import shlex
import sysconfig
import tempfile
import test.support
import test.test_tools
import test.support.script_helper
import unittest

_CPYTHON = pathlib.Path(test.support.REPO_ROOT).resolve()
_TOOLS_JIT = _CPYTHON / "Tools" / "jit"
_TOOLS_JIT_TEST = _TOOLS_JIT / "test"
_TOOLS_JIT_TEST_TEST_EXECUTOR_CASES_C_H = _TOOLS_JIT_TEST / "test_executor_cases.c.h"
_TOOLS_JIT_BUILD_PY = _TOOLS_JIT / "build.py"

test.test_tools.skip_if_missing("jit")
with test.test_tools.imports_under_tool("jit"):
    import _llvm

@test.support.cpython_only
@unittest.skipIf(test.support.Py_DEBUG, "Debug stencils aren't tested.")
@unittest.skipIf(test.support.Py_GIL_DISABLED, "Free-threaded stencils aren't tested.")
class TestJITStencils(unittest.TestCase):

    def _build_jit_stencils(self, target: str) -> str:
        with tempfile.TemporaryDirectory() as work:
            jit_stencils_h = pathlib.Path(work, f"jit_stencils-{target}.h").resolve()
            pyconfig_h = pathlib.Path(sysconfig.get_config_h_filename()).resolve()
            result, args = test.support.script_helper.run_python_until_end(
                _TOOLS_JIT_BUILD_PY,
                "--input-file", _TOOLS_JIT_TEST_TEST_EXECUTOR_CASES_C_H,
                "--output-dir", jit_stencils_h.parent,
                "--pyconfig-dir", pyconfig_h.parent,
                target,
                __isolated=False,
                # Windows leaks temporary files on failure because the JIT build
                # process is async. This forces it to be "sync" for this test:
                PYTHON_CPU_COUNT="1",
            )
            if result.rc:
                self.skipTest(f"Build failed: {shlex.join(map(str, args))}")
            body = jit_stencils_h.read_text()
        # Strip out two lines of header comments:
        _, _, body = body.split("\n", 2)
        return body

    def _check_jit_stencils(
        self, expected: str, actual: str, test_jit_stencils_h: pathlib.Path
    ) -> None:
        try:
            self.assertEqual(expected.strip("\n"), actual.strip("\n"))
        except AssertionError as e:
            # Make it easy to re-validate the expected output:
            relative = test_jit_stencils_h.relative_to(_CPYTHON)
            message = f"If this is expected, replace {relative} with:"
            banner = "=" * len(message)
            e.add_note("\n".join([banner, message, banner]))
            e.add_note(actual)
            raise

    def test_jit_stencils(self):
        if not asyncio.run(_llvm._find_tool("clang")):
            self.skipTest(f"LLVM {_llvm._LLVM_VERSION} isn't installed.")
        self.maxDiff = None
        found = False
        for test_jit_stencils_h in _TOOLS_JIT_TEST.glob("test_jit_stencils-*.h"):
            target = test_jit_stencils_h.stem.removeprefix("test_jit_stencils-")
            with self.subTest(target):
                expected = test_jit_stencils_h.read_text()
                actual = self._build_jit_stencils(target)
                found = True
                self._check_jit_stencils(expected, actual, test_jit_stencils_h)
        # This is a local build. If the JIT is available, at least one test should run:
        assert found, "No JIT stencils built!"


if __name__ == "__main__":
    unittest.main()
