import textwrap

from packaging.tests import unittest, support
from packaging.compat import Mixin2to3


class Mixin2to3TestCase(support.TempdirManager,
                        support.LoggingCatcher,
                        unittest.TestCase):

    @support.skip_2to3_optimize
    def test_convert_code_only(self):
        # used to check if code gets converted properly.
        code = "print 'test'"

        with self.mktempfile() as fp:
            fp.write(code)

        mixin2to3 = Mixin2to3()
        mixin2to3._run_2to3([fp.name])
        expected = "print('test')"

        with open(fp.name) as fp:
            converted = fp.read()

        self.assertEqual(expected, converted)

    def test_doctests_only(self):
        # used to check if doctests gets converted properly.
        doctest = textwrap.dedent('''\
            """Example docstring.

            >>> print test
            test

            It works.
            """''')

        with self.mktempfile() as fp:
            fp.write(doctest)

        mixin2to3 = Mixin2to3()
        mixin2to3._run_2to3([fp.name])
        expected = textwrap.dedent('''\
            """Example docstring.

            >>> print(test)
            test

            It works.
            """\n''')

        with open(fp.name) as fp:
            converted = fp.read()

        self.assertEqual(expected, converted)

    def test_additional_fixers(self):
        # used to check if use_2to3_fixers works
        code = 'type(x) is not T'

        with self.mktempfile() as fp:
            fp.write(code)

        mixin2to3 = Mixin2to3()
        mixin2to3._run_2to3(files=[fp.name], doctests=[fp.name],
                            fixers=['packaging.tests.fixer'])

        expected = 'not isinstance(x, T)'

        with open(fp.name) as fp:
            converted = fp.read()

        self.assertEqual(expected, converted)


def test_suite():
    return unittest.makeSuite(Mixin2to3TestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
