import textwrap

from packaging.tests import unittest, support
from packaging.compat import Mixin2to3


class Mixin2to3TestCase(support.TempdirManager,
                        support.LoggingCatcher,
                        unittest.TestCase):

    def setUp(self):
        super(Mixin2to3TestCase, self).setUp()
        self.filename = self.mktempfile().name

    def check(self, source, wanted, **kwargs):
        source = textwrap.dedent(source)
        with open(self.filename, 'w') as fp:
            fp.write(source)

        Mixin2to3()._run_2to3(**kwargs)

        wanted = textwrap.dedent(wanted)
        with open(self.filename) as fp:
            converted = fp.read()
        self.assertMultiLineEqual(converted, wanted)

    def test_conversion(self):
        # check that code and doctests get converted
        self.check('''\
            """Example docstring.

            >>> print test
            test

            It works.
            """
            print 'test'
            ''',
            '''\
            """Example docstring.

            >>> print(test)
            test

            It works.
            """
            print('test')

            ''',  # 2to3 adds a newline here
            files=[self.filename])

    def test_doctests_conversion(self):
        # check that doctest files are converted
        self.check('''\
            Welcome to the doc.

            >>> print test
            test
            ''',
            '''\
            Welcome to the doc.

            >>> print(test)
            test

            ''',
            doctests=[self.filename])

    def test_additional_fixers(self):
        # make sure the fixers argument works
        self.check("""\
            echo('42')
            echo2('oh no')
            """,
            """\
            print('42')
            print('oh no')
            """,
            files=[self.filename],
            fixers=['packaging.tests.fixer'])


def test_suite():
    return unittest.makeSuite(Mixin2to3TestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
