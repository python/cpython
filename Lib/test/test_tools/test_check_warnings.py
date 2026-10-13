import importlib
import unittest
from pathlib import Path
from test.support import captured_stdout, os_helper

from test.test_tools import imports_under_tool, skip_if_missing


skip_if_missing("build")
with imports_under_tool("build"):
    check_warnings = importlib.import_module("check_warnings")


class CheckWarningsTests(unittest.TestCase):
    def test_partial_improvement(self):
        for expected, actual, ignore_all, status in (
            (2, 1, False, 1), (2, 0, False, 1),
            (2, 2, False, 0), (2, 3, False, 0),
            (0, 0, False, 0), (2, 1, True, 0),
        ):
            with self.subTest(expected=expected, actual=actual,
                              ignore_all=ignore_all):
                rules = {check_warnings.IgnoreRule(
                    "source.c", expected, ignore_all)}
                warnings = {"source.c": [{}] * actual} if actual else {}
                with captured_stdout():
                    result = check_warnings.get_unexpected_improvements(
                        rules, warnings)
                self.assertEqual(result, status)

    def test_cli_partial_improvement(self):
        with os_helper.temp_dir() as directory:
            output = Path(directory, "compiler.txt")
            output.write_text("source.c:1:2: warning: example [-Wunused]\n",
                              encoding="utf-8")
            ignore = Path(directory, "ignore.txt")
            ignore.write_text("source.c 2\n", encoding="utf-8")
            args = ["-c", str(output), "-i", str(ignore), "-t", "gcc", "-p", ""]
            with captured_stdout():
                self.assertEqual(check_warnings.main(args), 0)
                self.assertEqual(check_warnings.main(
                    args + ["--fail-on-improvement"]), 1)


if __name__ == "__main__":
    unittest.main()
