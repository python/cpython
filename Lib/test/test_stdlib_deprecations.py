import unittest
import stdlib_deprecations


class Tests(unittest.TestCase):
    def test_python_api(self):
        obj = stdlib_deprecations.get_deprecated('asyncore')
        self.assertEqual(obj.name, 'asyncore')
        self.assertEqual(obj.version, (3, 6))
        self.assertEqual(obj.remove, (3, 12))
        self.assertEqual(obj.message, 'use asyncio')

        self.assertIs(stdlib_deprecations.get_deprecated('asyncore.loop'),
                      stdlib_deprecations.get_deprecated('asyncore'))

        self.assertIsNone(stdlib_deprecations.get_deprecated('builtins.open'))

    def test_c_api(self):
        obj = stdlib_deprecations.get_capi_deprecated('Py_VerboseFlag')
        self.assertEqual(obj.name, 'Py_VerboseFlag')
        self.assertEqual(obj.version, (3, 12))
        self.assertIsNone(obj.remove)
        self.assertEqual(obj.message, 'use PyConfig.verbose')

        self.assertIsNotNone(stdlib_deprecations.get_capi_deprecated('Py_VerboseFlag'))
        self.assertIsNotNone(stdlib_deprecations.get_capi_deprecated('PyUnicode_InternImmortal'))

        self.assertIsNone(stdlib_deprecations.get_capi_deprecated('Py_Initialize'))



if __name__ == '__main__':
    unittest.main()
