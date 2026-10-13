import importlib
import unittest
from pathlib import Path
from test.support import captured_stdout, os_helper

from test.test_tools import imports_under_tool, skip_if_missing


skip_if_missing("build")
with imports_under_tool("build"):
    check_warnings = importlib.import_module("check_warnings")


class CheckWarningsTests(unittest.TestCase):
    def test_cli_without_path_prefix(self):
        with os_helper.temp_dir() as directory:
            output = Path(directory, "compiler.txt")
            output.write_text("source.c:1:2: warning: example [-Wunused]\n",
                              encoding="utf-8")
            ignore = Path(directory, "ignore.txt")
            ignore.write_text("source.c 1\n", encoding="utf-8")
            args = ["-c", str(output), "-i", str(ignore), "-t", "gcc"]
            with captured_stdout():
                self.assertEqual(check_warnings.main(args), 0)
                self.assertEqual(check_warnings.main(
                    args + ["--fail-on-regression"]), 0)


if __name__ == "__main__":
    unittest.main()
