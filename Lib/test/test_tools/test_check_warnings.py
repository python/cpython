import importlib
import unittest

from test.test_tools import imports_under_tool, skip_if_missing


skip_if_missing("build")
with imports_under_tool("build"):
    check_warnings = importlib.import_module("check_warnings")


class CheckWarningsTests(unittest.TestCase):
    def test_warning_options(self):
        for compiler, suffix, option in (
            ("gcc", "", ""),
            ("gcc", " [-Wunused-variable]", "-Wunused-variable"),
            ("clang", " [-Wunused-variable]", "-Wunused-variable"),
        ):
            with self.subTest(compiler=compiler, suffix=suffix):
                warnings = check_warnings.extract_warnings_from_compiler_output(
                    "source.c:1:2: warning: example" + suffix, compiler)
                self.assertEqual(warnings, [{
                    "file": "source.c", "line": "1", "column": "2",
                    "message": "example", "option": option,
                }])


if __name__ == "__main__":
    unittest.main()
