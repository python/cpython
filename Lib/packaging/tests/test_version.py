"""Tests for packaging.version."""
import doctest
import os

from packaging.version import NormalizedVersion as V
from packaging.version import HugeMajorVersionNumError, IrrationalVersionError
from packaging.version import suggest_normalized_version as suggest
from packaging.version import VersionPredicate
from packaging.tests import unittest


class VersionTestCase(unittest.TestCase):

    versions = ((V('1.0'), '1.0'),
                (V('1.1'), '1.1'),
                (V('1.2.3'), '1.2.3'),
                (V('1.2'), '1.2'),
                (V('1.2.3a4'), '1.2.3a4'),
                (V('1.2c4'), '1.2c4'),
                (V('1.2.3.4'), '1.2.3.4'),
                (V('1.2.3.4.0b3'), '1.2.3.4b3'),
                (V('1.2.0.0.0'), '1.2'),
                (V('1.0.dev345'), '1.0.dev345'),
                (V('1.0.post456.dev623'), '1.0.post456.dev623'))

    def test_repr(self):

        self.assertEqual(repr(V('1.0')), "NormalizedVersion('1.0')")

    def test_basic_versions(self):

        for v, s in self.versions:
            self.assertEqual(str(v), s)

    def test_hash(self):

        for v, s in self.versions:
            self.assertEqual(hash(v), hash(V(s)))

        versions = set([v for v, s in self.versions])
        for v, s in self.versions:
            self.assertIn(v, versions)

        self.assertEqual(set([V('1.0')]), set([V('1.0'), V('1.0')]))

    def test_from_parts(self):

        for v, s in self.versions:
            parts = v.parts
            v2 = V.from_parts(*v.parts)
            self.assertEqual(v, v2)
            self.assertEqual(str(v), str(v2))

    def test_irrational_versions(self):

        irrational = ('1', '1.2a', '1.2.3b', '1.02', '1.2a03',
                      '1.2a3.04', '1.2.dev.2', '1.2dev', '1.2.dev',
                      '1.2.dev2.post2', '1.2.post2.dev3.post4')

        for s in irrational:
            self.assertRaises(IrrationalVersionError, V, s)

    def test_huge_version(self):

        self.assertEqual(str(V('1980.0')), '1980.0')
        self.assertRaises(HugeMajorVersionNumError, V, '1981.0')
        self.assertEqual(str(V('1981.0', error_on_huge_major_num=False)),
                         '1981.0')

    def test_comparison(self):
        comparison_doctest_string = r"""
        >>> V('1.2.0') == '1.2'
        Traceback (most recent call last):
        ...
        TypeError: cannot compare NormalizedVersion and str

        >>> V('1.2') < '1.3'
        Traceback (most recent call last):
        ...
        TypeError: cannot compare NormalizedVersion and str

        >>> V('1.2.0') == V('1.2')
        True
        >>> V('1.2.0') == V('1.2.3')
        False
        >>> V('1.2.0') != V('1.2.3')
        True
        >>> V('1.2.0') < V('1.2.3')
        True
        >>> V('1.2.0') < V('1.2.0')
        False
        >>> V('1.2.0') <= V('1.2.0')
        True
        >>> V('1.2.0') <= V('1.2.3')
        True
        >>> V('1.2.3') <= V('1.2.0')
        False
        >>> V('1.2.0') >= V('1.2.0')
        True
        >>> V('1.2.3') >= V('1.2.0')
        True
        >>> V('1.2.0') >= V('1.2.3')
        False
        >>> (V('1.0') > V('1.0b2'))
        True
        >>> (V('1.0') > V('1.0c2') > V('1.0c1') > V('1.0b2') > V('1.0b1')
        ...  > V('1.0a2') > V('1.0a1'))
        True
        >>> (V('1.0.0') > V('1.0.0c2') > V('1.0.0c1') > V('1.0.0b2') > V('1.0.0b1')
        ...  > V('1.0.0a2') > V('1.0.0a1'))
        True

        >>> V('1.0') < V('1.0.post456.dev623')
        True

        >>> V('1.0.post456.dev623') < V('1.0.post456')  < V('1.0.post1234')
        True

        >>> (V('1.0a1')
        ...  < V('1.0a2.dev456')
        ...  < V('1.0a2')
        ...  < V('1.0a2.1.dev456')  # e.g. need to do a quick post release on 1.0a2
        ...  < V('1.0a2.1')
        ...  < V('1.0b1.dev456')
        ...  < V('1.0b2')
        ...  < V('1.0c1.dev456')
        ...  < V('1.0c1')
        ...  < V('1.0.dev7')
        ...  < V('1.0.dev18')
        ...  < V('1.0.dev456')
        ...  < V('1.0.dev1234')
        ...  < V('1.0')
        ...  < V('1.0.post456.dev623')  # development version of a post release
        ...  < V('1.0.post456'))
        True
        """
        doctest.script_from_examples(comparison_doctest_string)

    def test_suggest_normalized_version(self):

        self.assertEqual(suggest('1.0'), '1.0')
        self.assertEqual(suggest('1.0-alpha1'), '1.0a1')
        self.assertEqual(suggest('1.0c2'), '1.0c2')
        self.assertEqual(suggest('walla walla washington'), None)
        self.assertEqual(suggest('2.4c1'), '2.4c1')
        self.assertEqual(suggest('v1.0'), '1.0')

        # from setuptools
        self.assertEqual(suggest('0.4a1.r10'), '0.4a1.post10')
        self.assertEqual(suggest('0.7a1dev-r66608'), '0.7a1.dev66608')
        self.assertEqual(suggest('0.6a9.dev-r41475'), '0.6a9.dev41475')
        self.assertEqual(suggest('2.4preview1'), '2.4c1')
        self.assertEqual(suggest('2.4pre1'), '2.4c1')
        self.assertEqual(suggest('2.1-rc2'), '2.1c2')

        # from pypi
        self.assertEqual(suggest('0.1dev'), '0.1.dev0')
        self.assertEqual(suggest('0.1.dev'), '0.1.dev0')

        # we want to be able to parse Twisted
        # development versions are like post releases in Twisted
        self.assertEqual(suggest('9.0.0+r2363'), '9.0.0.post2363')

        # pre-releases are using markers like "pre1"
        self.assertEqual(suggest('9.0.0pre1'), '9.0.0c1')

        # we want to be able to parse Tcl-TK
        # they us "p1" "p2" for post releases
        self.assertEqual(suggest('1.4p1'), '1.4.post1')

    def test_predicate(self):
        # VersionPredicate knows how to parse stuff like:
        #
        #   Project (>=version, ver2)

        predicates = ('zope.interface (>3.5.0)',
                      'AnotherProject (3.4)',
                      'OtherProject (<3.0)',
                      'NoVersion',
                      'Hey (>=2.5,<2.7)')

        for predicate in predicates:
            v = VersionPredicate(predicate)

        self.assertTrue(VersionPredicate('Hey (>=2.5,<2.7)').match('2.6'))
        self.assertTrue(VersionPredicate('Ho').match('2.6'))
        self.assertFalse(VersionPredicate('Hey (>=2.5,!=2.6,<2.7)').match('2.6'))
        self.assertTrue(VersionPredicate('Ho (<3.0)').match('2.6'))
        self.assertTrue(VersionPredicate('Ho (<3.0,!=2.5)').match('2.6.0'))
        self.assertFalse(VersionPredicate('Ho (<3.0,!=2.6)').match('2.6.0'))
        self.assertTrue(VersionPredicate('Ho (2.5)').match('2.5.4'))
        self.assertFalse(VersionPredicate('Ho (!=2.5)').match('2.5.2'))
        self.assertTrue(VersionPredicate('Hey (<=2.5)').match('2.5.9'))
        self.assertFalse(VersionPredicate('Hey (<=2.5)').match('2.6.0'))
        self.assertTrue(VersionPredicate('Hey (>=2.5)').match('2.5.1'))

        self.assertRaises(ValueError, VersionPredicate, '')

        self.assertTrue(VersionPredicate('Hey 2.5').match('2.5.1'))

        # XXX need to silent the micro version in this case
        self.assertFalse(VersionPredicate('Ho (<3.0,!=2.6)').match('2.6.3'))

        # Make sure a predicate that ends with a number works
        self.assertTrue(VersionPredicate('virtualenv5 (1.0)').match('1.0'))
        self.assertTrue(VersionPredicate('virtualenv5').match('1.0'))
        self.assertTrue(VersionPredicate('vi5two').match('1.0'))
        self.assertTrue(VersionPredicate('5two').match('1.0'))
        self.assertTrue(VersionPredicate('vi5two 1.0').match('1.0'))
        self.assertTrue(VersionPredicate('5two 1.0').match('1.0'))

        # test repr
        for predicate in predicates:
            self.assertEqual(str(VersionPredicate(predicate)), predicate)

    def test_predicate_name(self):
        # Test that names are parsed the right way

        self.assertEqual('Hey', VersionPredicate('Hey (<1.1)').name)
        self.assertEqual('Foo-Bar', VersionPredicate('Foo-Bar (1.1)').name)
        self.assertEqual('Foo Bar', VersionPredicate('Foo Bar (1.1)').name)

    def test_is_final(self):
        # VersionPredicate knows is a distribution is a final one or not.
        final_versions = ('1.0', '1.0.post456')
        other_versions = ('1.0.dev1', '1.0a2', '1.0c3')

        for version in final_versions:
            self.assertTrue(V(version).is_final)
        for version in other_versions:
            self.assertFalse(V(version).is_final)


class VersionWhiteBoxTestCase(unittest.TestCase):

    def test_parse_numdots(self):
        # For code coverage completeness, as pad_zeros_length can't be set or
        # influenced from the public interface
        self.assertEqual(V('1.0')._parse_numdots('1.0', '1.0',
                                                  pad_zeros_length=3),
                          [1, 0, 0])


def test_suite():
    #README = os.path.join(os.path.dirname(__file__), 'README.txt')
    #suite = [doctest.DocFileSuite(README), unittest.makeSuite(VersionTestCase)]
    suite = [unittest.makeSuite(VersionTestCase),
             unittest.makeSuite(VersionWhiteBoxTestCase)]
    return unittest.TestSuite(suite)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
