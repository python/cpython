"""Tests for Argument Clinic using the interpreter that runs the test suite."""

import os
from pathlib import Path
import tempfile
import unittest

from test.support.script_helper import assert_python_ok, assert_python_failure
from test.test_tools import skip_if_missing, toolsdir

skip_if_missing()
CLINIC = os.path.join(toolsdir, 'clinic', 'clinic.py')


class ClinicTests(unittest.TestCase):
    def generate(self, parameters, *, success=True):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'test.c'
            source.write_text(
                '/*[clinic input]\nmodule test\ntest.func\n' + parameters +
                '    /\n[clinic start generated code]*/\n', encoding='utf-8')
            # Clinic imports cpp.py from its own directory.
            if not success:
                return assert_python_failure(
                    CLINIC, str(source), __isolated=False)[1]
            assert_python_ok(CLINIC, str(source), __isolated=False)
            header = Path(directory) / 'clinic' / 'test.c.h'
            generated_source = source.read_bytes()
            generated_header = header.read_bytes()
            # Regeneration must also validate the checksums it just wrote.
            assert_python_ok(CLINIC, str(source), __isolated=False)
            self.assertEqual(source.read_bytes(), generated_source)
            self.assertEqual(header.read_bytes(), generated_header)
            return generated_header.decode('utf-8')

    def test_negative_numeric_defaults(self):
        header = self.generate(
            '    value: int = -1\n'
            '    count: Py_ssize_t = -1\n'
            '    ratio: double = -1.5\n')
        self.assertIn('int value = -1;', header)
        self.assertIn('Py_ssize_t count = -1;', header)
        self.assertIn('double ratio = -1.5;', header)

    def test_legacy_string_converter(self):
        header = self.generate('    text: "s"\n')
        self.assertIn('const char *text;', header)
        self.assertIn('"s:func"', header)

    def test_null_string_default(self):
        header = self.generate(
            '    errors: str(accept={str, NoneType}) = NULL\n')
        self.assertIn('errors=None', header)
        self.assertIn('const char *errors = NULL;', header)

    def test_expression_requires_c_default(self):
        output = self.generate('    value: int = 1 + 2\n', success=False)
        self.assertIn(b'you MUST specify a valid c_default', output)


if __name__ == '__main__':
    unittest.main()
