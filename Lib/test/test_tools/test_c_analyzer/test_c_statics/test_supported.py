import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser import info
    from c_statics.supported import is_supported


class IsSupportedTests(unittest.TestCase):

    @unittest.expectedFailure
    def test_supported(self):
        statics = [
                info.StaticVar('src1/spam.c', None, 'var1', 'const char *'),
                info.StaticVar('src1/spam.c', None, 'var1', 'int'),
                ]
        for static in statics:
            with self.subTest(static):
                result = is_supported(static)

                self.assertTrue(result)

    @unittest.expectedFailure
    def test_not_supported(self):
        statics = [
                info.StaticVar('src1/spam.c', None, 'var1', 'PyObject *'),
                info.StaticVar('src1/spam.c', None, 'var1', 'PyObject[10]'),
                ]
        for static in statics:
            with self.subTest(static):
                result = is_supported(static)

                self.assertFalse(result)
