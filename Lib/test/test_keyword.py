import keyword
import unittest
from test import support
import filecmp
import os
import sys
import subprocess
import shutil
import textwrap

KEYWORD_FILE             = support.findfile('keyword.py')
GRAMMAR_FILE             = os.path.join('..', '..', 'Python', 'graminit.c')
TEST_PY_FILE             = 'keyword_test.py'
GRAMMAR_TEST_FILE        = 'graminit_test.c'
PY_FILE_WITHOUT_KEYWORDS = 'minimal_keyword.py'
NONEXISTENT_FILE         = 'not_here.txt'


class Test_iskeyword(unittest.TestCase):
    def test_true_is_a_keyword(self):
        self.assertTrue(keyword.iskeyword('True'))

    def test_uppercase_true_is_not_a_keyword(self):
        self.assertFalse(keyword.iskeyword('TRUE'))

    def test_none_value_is_not_a_keyword(self):
        self.assertFalse(keyword.iskeyword(None))

    # This is probably an accident of the current implementation, but should be
    # preserved for backward compatibility.
    def test_changing_the_kwlist_does_not_affect_iskeyword(self):
        oldlist = keyword.kwlist
        self.addCleanup(lambda: setattr(keyword, 'kwlist', oldlist))
        keyword.kwlist = ['its', 'all', 'eggs', 'beans', 'and', 'a', 'slice']
        self.assertFalse(keyword.iskeyword('eggs'))


class TestKeywordGeneration(unittest.TestCase):

    def _copy_file_without_generated_keywords(self, source_file, dest_file):
        with open(source_file) as fp:
            lines = fp.readlines()
        with open(dest_file, 'w') as fp:
            fp.writelines(lines[:lines.index("#--start keywords--\n") + 1])
            fp.writelines(lines[lines.index("#--end keywords--\n"):])

    def _generate_keywords(self, grammar_file, target_keyword_py_file):
        proc = subprocess.Popen([sys.executable,
                                 KEYWORD_FILE,
                                 grammar_file,
                                 target_keyword_py_file], stderr=subprocess.PIPE)
        stderr = proc.communicate()[1]
        return proc.returncode, stderr

    @unittest.skipIf(not os.path.exists(GRAMMAR_FILE),
                     'test only works from source build directory')
    def test_real_grammar_and_keyword_file(self):
        self._copy_file_without_generated_keywords(KEYWORD_FILE, TEST_PY_FILE)
        self.addCleanup(lambda: support.unlink(TEST_PY_FILE))
        self.assertFalse(filecmp.cmp(KEYWORD_FILE, TEST_PY_FILE))
        self.assertEqual(0, self._generate_keywords(GRAMMAR_FILE,
                                                    TEST_PY_FILE)[0])
        self.assertTrue(filecmp.cmp(KEYWORD_FILE, TEST_PY_FILE))

    def test_grammar(self):
        self._copy_file_without_generated_keywords(KEYWORD_FILE, TEST_PY_FILE)
        self.addCleanup(lambda: support.unlink(TEST_PY_FILE))
        with open(GRAMMAR_TEST_FILE, 'w') as fp:
            # Some of these are probably implementation accidents.
            fp.writelines(textwrap.dedent("""\
                {2, 1},
                    {11, "encoding_decl", 0, 2, states_79,
                     "\000\000\040\000\000\000\000\000\000\000\000\000"
                     "\000\000\000\000\000\000\000\000\000"},
                    {1, "jello"},
                    {326, 0},
                    {1, "turnip"},
                \t{1, "This one is tab indented"
                    {278, 0},
                    {1, "crazy but legal"
                "also legal" {1, "
                    {1, "continue"},
                   {1, "lemon"},
                     {1, "tomato"},
                {1, "wigii"},
                    {1, 'no good'}
                    {283, 0},
                    {1,  "too many spaces"}"""))
        self.addCleanup(lambda: support.unlink(GRAMMAR_TEST_FILE))
        self._generate_keywords(GRAMMAR_TEST_FILE, TEST_PY_FILE)
        expected = [
            "        'This one is tab indented',\n",
            "        'also legal',\n",
            "        'continue',\n",
            "        'crazy but legal',\n",
            "        'jello',\n",
            "        'lemon',\n",
            "        'tomato',\n",
            "        'turnip',\n",
            "        'wigii',\n",
            ]
        with open(TEST_PY_FILE) as fp:
            lines = fp.readlines()
        start = lines.index("#--start keywords--\n") + 1
        end = lines.index("#--end keywords--\n")
        actual = lines[start:end]
        self.assertEqual(actual, expected)

    def test_empty_grammar_results_in_no_keywords(self):
        self._copy_file_without_generated_keywords(KEYWORD_FILE,
                                                   PY_FILE_WITHOUT_KEYWORDS)
        self.addCleanup(lambda: support.unlink(PY_FILE_WITHOUT_KEYWORDS))
        shutil.copyfile(KEYWORD_FILE, TEST_PY_FILE)
        self.addCleanup(lambda: support.unlink(TEST_PY_FILE))
        self.assertEqual(0, self._generate_keywords(os.devnull,
                                                    TEST_PY_FILE)[0])
        self.assertTrue(filecmp.cmp(TEST_PY_FILE, PY_FILE_WITHOUT_KEYWORDS))

    def test_keywords_py_without_markers_produces_error(self):
        rc, stderr = self._generate_keywords(os.devnull, os.devnull)
        self.assertNotEqual(rc, 0)
        self.assertEqual(stderr, b'target does not contain format markers\n')

    def test_missing_grammar_file_produces_error(self):
        rc, stderr = self._generate_keywords(NONEXISTENT_FILE, KEYWORD_FILE)
        self.assertNotEqual(rc, 0)
        self.assertRegex(stderr, b'(?ms)' + NONEXISTENT_FILE.encode())

    def test_missing_keywords_py_file_produces_error(self):
        rc, stderr = self._generate_keywords(os.devnull, NONEXISTENT_FILE)
        self.assertNotEqual(rc, 0)
        self.assertRegex(stderr, b'(?ms)' + NONEXISTENT_FILE.encode())


if __name__ == "__main__":
    unittest.main()
