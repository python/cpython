import unittest
from test import support
import filecmp
import os
import sys
import subprocess


SYMBOL_FILE              = support.findfile('symbol.py')
GRAMMAR_FILE             = os.path.join(os.path.dirname(__file__),
                                        '..', '..', 'Include', 'graminit.h')
TEST_PY_FILE             = 'symbol_test.py'


class TestSymbolGeneration(unittest.TestCase):

    def _copy_file_without_generated_symbols(self, source_file, dest_file):
        with open(source_file, 'rb') as fp:
            lines = fp.readlines()
        nl = lines[0][len(lines[0].rstrip()):]
        with open(dest_file, 'wb') as fp:
            fp.writelines(lines[:lines.index(b"#--start constants--" + nl) + 1])
            fp.writelines(lines[lines.index(b"#--end constants--" + nl):])

    def _generate_symbols(self, grammar_file, target_symbol_py_file):
        proc = subprocess.Popen([sys.executable,
                                 SYMBOL_FILE,
                                 grammar_file,
                                 target_symbol_py_file], stderr=subprocess.PIPE)
        stderr = proc.communicate()[1]
        return proc.returncode, stderr

    @unittest.skipIf(not os.path.exists(GRAMMAR_FILE),
                     'test only works from source build directory')
    def test_real_grammar_and_symbol_file(self):
        self._copy_file_without_generated_symbols(SYMBOL_FILE, TEST_PY_FILE)
        self.addCleanup(support.unlink, TEST_PY_FILE)
        self.assertFalse(filecmp.cmp(SYMBOL_FILE, TEST_PY_FILE))
        self.assertEqual((0, b''), self._generate_symbols(GRAMMAR_FILE,
                                                          TEST_PY_FILE))
        self.assertTrue(filecmp.cmp(SYMBOL_FILE, TEST_PY_FILE))


if __name__ == "__main__":
    unittest.main()
