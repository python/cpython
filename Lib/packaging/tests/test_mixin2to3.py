"""Tests for packaging.command.build_py."""
import sys

from packaging.tests import unittest, support
from packaging.compat import Mixin2to3


class Mixin2to3TestCase(support.TempdirManager,
                        support.LoggingCatcher,
                        unittest.TestCase):

    @unittest.skipIf(sys.version < '2.6', 'requires Python 2.6 or higher')
    def test_convert_code_only(self):
        # used to check if code gets converted properly.
        code_content = "print 'test'\n"
        code_handle = self.mktempfile()
        code_name = code_handle.name

        code_handle.write(code_content)
        code_handle.flush()

        mixin2to3 = Mixin2to3()
        mixin2to3._run_2to3([code_name])
        converted_code_content = "print('test')\n"
        with open(code_name) as fp:
            new_code_content = "".join(fp.readlines())

        self.assertEqual(new_code_content, converted_code_content)

    @unittest.skipIf(sys.version < '2.6', 'requires Python 2.6 or higher')
    def test_doctests_only(self):
        # used to check if doctests gets converted properly.
        doctest_content = '"""\n>>> print test\ntest\n"""\nprint test\n\n'
        doctest_handle = self.mktempfile()
        doctest_name = doctest_handle.name

        doctest_handle.write(doctest_content)
        doctest_handle.flush()

        mixin2to3 = Mixin2to3()
        mixin2to3._run_2to3([doctest_name])

        converted_doctest_content = ['"""', '>>> print(test)', 'test', '"""',
                                     'print(test)', '', '', '']
        converted_doctest_content = '\n'.join(converted_doctest_content)
        with open(doctest_name) as fp:
            new_doctest_content = "".join(fp.readlines())

        self.assertEqual(new_doctest_content, converted_doctest_content)

    @unittest.skipIf(sys.version < '2.6', 'requires Python 2.6 or higher')
    def test_additional_fixers(self):
        # used to check if use_2to3_fixers works
        code_content = "type(x) is T"
        code_handle = self.mktempfile()
        code_name = code_handle.name

        code_handle.write(code_content)
        code_handle.flush()

        mixin2to3 = Mixin2to3()

        mixin2to3._run_2to3(files=[code_name], doctests=[code_name],
                            fixers=['packaging.tests.fixer'])
        converted_code_content = "isinstance(x, T)"
        with open(code_name) as fp:
            new_code_content = "".join(fp.readlines())
        self.assertEqual(new_code_content, converted_code_content)


def test_suite():
    return unittest.makeSuite(Mixin2to3TestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
