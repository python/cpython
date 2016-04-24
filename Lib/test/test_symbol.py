import unittest
from test import support
import os
import sys
import subprocess


SYMBOL_FILE              = support.findfile('symbol.py')
GRAMMAR_FILE             = os.path.join(os.path.dirname(__file__),
                                        '..', '..', 'Include', 'graminit.h')
TEST_PY_FILE             = 'symbol_test.py'


class TestSymbolGeneration(unittest.TestCase):

    def _copy_file_without_generated_symbols(self, source_file, dest_file):
        with open(source_file) as fp:
            lines = fp.readlines()
        with open(dest_file, 'w') as fp:
            fp.writelines(lines[:lines.index("#--start constants--\n") + 1])
            fp.writelines(lines[lines.index("#--end constants--\n"):])

    def _generate_symbols(self, grammar_file, target_symbol_py_file):
        proc = subprocess.Popen([sys.executable,
                                 SYMBOL_FILE,
                                 grammar_file,
                                 target_symbol_py_file], stderr=subprocess.PIPE)
        stderr = proc.communicate()[1]
        return proc.returncode, stderr

    def compare_files(self, file1, file2):
        with open(file1) as fp:
            lines1 = fp.readlines()
        with open(file2) as fp:
            lines2 = fp.readlines()
        self.assertEqual(lines1, lines2)

    @unittest.skipIf(not os.path.exists(GRAMMAR_FILE),
                     'test only works from source build directory')
    def test_real_grammar_and_symbol_file(self):
        output = support.TESTFN
        self.addCleanup(support.unlink, output)

        self._copy_file_without_generated_symbols(SYMBOL_FILE, output)

        exitcode, stderr = self._generate_symbols(GRAMMAR_FILE, output)
        self.assertEqual(b'', stderr)
        self.assertEqual(0, exitcode)

        self.compare_files(SYMBOL_FILE, output)


if __name__ == "__main__":
    unittest.main()
