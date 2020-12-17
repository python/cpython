# Copyright (c) Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Tests for L{incremental}.
"""

from __future__ import division, absolute_import

import operator

from incremental import getVersionString, IncomparableVersions
from incremental import Version, _inf

from twisted.trial.unittest import TestCase


class VersionsTests(TestCase):

    def test_localIsShort(self):
        """
        The local version is the same as the short version.
        """
        va = Version("dummy", 1, 0, 0, release_candidate=1, dev=3)
        self.assertEqual(va.local(), va.short())

    def test_versionComparison(self):
        """
        Versions can be compared for equality and order.
        """
        va = Version("dummy", 1, 0, 0)
        vb = Version("dummy", 0, 1, 0)
        self.assertTrue(va > vb)
        self.assertTrue(vb < va)
        self.assertTrue(va >= vb)
        self.assertTrue(vb <= va)
        self.assertTrue(va != vb)
        self.assertTrue(vb == Version("dummy", 0, 1, 0))
        self.assertTrue(vb == vb)

    def test_versionComparisonCaseInsensitive(self):
        """
        Version package names are case insensitive.
        """
        va = Version("dummy", 1, 0, 0)
        vb = Version("DuMmY", 0, 1, 0)
        self.assertTrue(va > vb)
        self.assertTrue(vb < va)
        self.assertTrue(va >= vb)
        self.assertTrue(vb <= va)
        self.assertTrue(va != vb)
        self.assertTrue(vb == Version("dummy", 0, 1, 0))
        self.assertTrue(vb == vb)

    def test_comparingNEXTReleases(self):
        """
        NEXT releases are always larger than numbered releases.
        """
        va = Version("whatever", "NEXT", 0, 0)
        vb = Version("whatever", 1, 0, 0)
        self.assertTrue(va > vb)
        self.assertFalse(va < vb)
        self.assertNotEquals(vb, va)

    def test_NEXTMustBeAlone(self):
        """
        NEXT releases must always have the rest of the numbers set to 0.
        """
        with self.assertRaises(ValueError):
            Version("whatever", "NEXT", 1, 0, release_candidate=0, dev=0)

        with self.assertRaises(ValueError):
            Version("whatever", "NEXT", 0, 1, release_candidate=0, dev=0)

        with self.assertRaises(ValueError):
            Version("whatever", "NEXT", 0, 0, release_candidate=1, dev=0)

        with self.assertRaises(ValueError):
            Version("whatever", "NEXT", 0, 0, release_candidate=0, dev=1)

    def test_comparingNEXTReleasesEqual(self):
        """
        NEXT releases are equal to each other.
        """
        va = Version("whatever", "NEXT", 0, 0)
        vb = Version("whatever", "NEXT", 0, 0)
        self.assertEquals(vb, va)

    def test_comparingPrereleasesWithReleases(self):
        """
        Prereleases are always less than versions without prereleases.
        """
        va = Version("whatever", 1, 0, 0, prerelease=1)
        vb = Version("whatever", 1, 0, 0)
        self.assertTrue(va < vb)
        self.assertFalse(va > vb)
        self.assertNotEquals(vb, va)

    def test_prereleaseDeprecated(self):
        """
        Passing 'prerelease' to Version is deprecated.
        """
        Version("whatever", 1, 0, 0, prerelease=1)
        warnings = self.flushWarnings([self.test_prereleaseDeprecated])
        self.assertEqual(len(warnings), 1)
        self.assertEqual(
            warnings[0]['message'],
            ("Passing prerelease to incremental.Version was deprecated in "
             "Incremental 16.9.0. Please pass release_candidate instead."))

    def test_prereleaseAttributeDeprecated(self):
        """
        Accessing 'prerelease' on a Version is deprecated.
        """
        va = Version("whatever", 1, 0, 0, release_candidate=1)
        va.prerelease
        warnings = self.flushWarnings(
            [self.test_prereleaseAttributeDeprecated])
        self.assertEqual(len(warnings), 1)
        self.assertEqual(
            warnings[0]['message'],
            ("Accessing incremental.Version.prerelease was deprecated in "
             "Incremental 16.9.0. Use Version.release_candidate instead."))

    def test_comparingReleaseCandidatesWithReleases(self):
        """
        Release Candidates are always less than versions without release
        candidates.
        """
        va = Version("whatever", 1, 0, 0, release_candidate=1)
        vb = Version("whatever", 1, 0, 0)
        self.assertTrue(va < vb)
        self.assertFalse(va > vb)
        self.assertNotEquals(vb, va)

    def test_comparingDevReleasesWithReleases(self):
        """
        Dev releases are always less than versions without dev releases.
        """
        va = Version("whatever", 1, 0, 0, dev=1)
        vb = Version("whatever", 1, 0, 0)
        self.assertTrue(va < vb)
        self.assertFalse(va > vb)
        self.assertNotEquals(vb, va)

    def test_rcEqualspre(self):
        """
        Release Candidates are equal to prereleases.
        """
        va = Version("whatever", 1, 0, 0, release_candidate=1)
        vb = Version("whatever", 1, 0, 0, prerelease=1)
        self.assertTrue(va == vb)
        self.assertFalse(va != vb)

    def test_rcOrpreButNotBoth(self):
        """
        Release Candidate and prerelease can't both be given.
        """
        with self.assertRaises(ValueError):
            Version("whatever", 1, 0, 0,
                    prerelease=1, release_candidate=1)

    def test_comparingReleaseCandidates(self):
        """
        The value specified as the release candidate is used in version
        comparisons.
        """
        va = Version("whatever", 1, 0, 0, release_candidate=1)
        vb = Version("whatever", 1, 0, 0, release_candidate=2)
        self.assertTrue(va < vb)
        self.assertTrue(vb > va)
        self.assertTrue(va <= vb)
        self.assertTrue(vb >= va)
        self.assertTrue(va != vb)
        self.assertTrue(vb == Version("whatever", 1, 0, 0,
                                      release_candidate=2))
        self.assertTrue(va == va)

    def test_comparingDev(self):
        """
        The value specified as the dev release is used in version comparisons.
        """
        va = Version("whatever", 1, 0, 0, dev=1)
        vb = Version("whatever", 1, 0, 0, dev=2)
        self.assertTrue(va < vb)
        self.assertTrue(vb > va)
        self.assertTrue(va <= vb)
        self.assertTrue(vb >= va)
        self.assertTrue(va != vb)
        self.assertTrue(vb == Version("whatever", 1, 0, 0,
                                      dev=2))
        self.assertTrue(va == va)

    def test_comparingDevAndRC(self):
        """
        The value specified as the dev release and release candidate is used in
        version comparisons.
        """
        va = Version("whatever", 1, 0, 0, release_candidate=1, dev=1)
        vb = Version("whatever", 1, 0, 0, release_candidate=1, dev=2)
        self.assertTrue(va < vb)
        self.assertTrue(vb > va)
        self.assertTrue(va <= vb)
        self.assertTrue(vb >= va)
        self.assertTrue(va != vb)
        self.assertTrue(vb == Version("whatever", 1, 0, 0,
                                      release_candidate=1, dev=2))
        self.assertTrue(va == va)

    def test_comparingDevAndRCDifferent(self):
        """
        The value specified as the dev release and release candidate is used in
        version comparisons.
        """
        va = Version("whatever", 1, 0, 0, release_candidate=1, dev=1)
        vb = Version("whatever", 1, 0, 0, release_candidate=2, dev=1)
        self.assertTrue(va < vb)
        self.assertTrue(vb > va)
        self.assertTrue(va <= vb)
        self.assertTrue(vb >= va)
        self.assertTrue(va != vb)
        self.assertTrue(vb == Version("whatever", 1, 0, 0,
                                      release_candidate=2, dev=1))
        self.assertTrue(va == va)

    def test_infComparison(self):
        """
        L{_inf} is equal to L{_inf}.

        This is a regression test.
        """
        self.assertEqual(_inf, _inf)

    def test_disallowBuggyComparisons(self):
        """
        The package names of the Version objects need to be the same.
        """
        self.assertRaises(IncomparableVersions,
                          operator.eq,
                          Version("dummy", 1, 0, 0),
                          Version("dumym", 1, 0, 0))

    def test_notImplementedComparisons(self):
        """
        Comparing a L{Version} to some other object type results in
        C{NotImplemented}.
        """
        va = Version("dummy", 1, 0, 0)
        vb = ("dummy", 1, 0, 0)  # a tuple is not a Version object
        self.assertEqual(va.__cmp__(vb), NotImplemented)

    def test_repr(self):
        """
        Calling C{repr} on a version returns a human-readable string
        representation of the version.
        """
        self.assertEqual(repr(Version("dummy", 1, 2, 3)),
                         "Version('dummy', 1, 2, 3)")

    def test_reprWithPrerelease(self):
        """
        Calling C{repr} on a version with a prerelease returns a human-readable
        string representation of the version including the prerelease as a
        release candidate..
        """
        self.assertEqual(repr(Version("dummy", 1, 2, 3, prerelease=4)),
                         "Version('dummy', 1, 2, 3, release_candidate=4)")

    def test_reprWithReleaseCandidate(self):
        """
        Calling C{repr} on a version with a release candidate returns a
        human-readable string representation of the version including the rc.
        """
        self.assertEqual(repr(Version("dummy", 1, 2, 3, release_candidate=4)),
                         "Version('dummy', 1, 2, 3, release_candidate=4)")

    def test_devWithReleaseCandidate(self):
        """
        Calling C{repr} on a version with a dev release returns a
        human-readable string representation of the version including the dev
        release.
        """
        self.assertEqual(repr(Version("dummy", 1, 2, 3, dev=4)),
                         "Version('dummy', 1, 2, 3, dev=4)")

    def test_str(self):
        """
        Calling C{str} on a version returns a human-readable string
        representation of the version.
        """
        self.assertEqual(str(Version("dummy", 1, 2, 3)),
                         "[dummy, version 1.2.3]")

    def test_strWithPrerelease(self):
        """
        Calling C{str} on a version with a prerelease includes the prerelease
        as a release candidate.
        """
        self.assertEqual(str(Version("dummy", 1, 0, 0, prerelease=1)),
                         "[dummy, version 1.0.0rc1]")

    def test_strWithReleaseCandidate(self):
        """
        Calling C{str} on a version with a release candidate includes the
        release candidate.
        """
        self.assertEqual(str(Version("dummy", 1, 0, 0, release_candidate=1)),
                         "[dummy, version 1.0.0rc1]")

    def test_strWithDevAndReleaseCandidate(self):
        """
        Calling C{str} on a version with a release candidate and dev release
        includes the release candidate and the dev release.
        """
        self.assertEqual(str(Version("dummy", 1, 0, 0,
                                     release_candidate=1, dev=2)),
                         "[dummy, version 1.0.0rc1dev2]")

    def test_strWithDev(self):
        """
        Calling C{str} on a version with a dev release includes the dev
        release.
        """
        self.assertEqual(str(Version("dummy", 1, 0, 0, dev=1)),
                         "[dummy, version 1.0.0dev1]")

    def testShort(self):
        self.assertEqual(Version('dummy', 1, 2, 3).short(), '1.2.3')

    def test_getVersionString(self):
        """
        L{getVersionString} returns a string with the package name and the
        short version number.
        """
        self.assertEqual(
            'Twisted 8.0.0', getVersionString(Version('Twisted', 8, 0, 0)))

    def test_getVersionStringWithPrerelease(self):
        """
        L{getVersionString} includes the prerelease as a release candidate, if
        any.
        """
        self.assertEqual(
            getVersionString(Version("whatever", 8, 0, 0, prerelease=1)),
            "whatever 8.0.0rc1")

    def test_getVersionStringWithReleaseCandidate(self):
        """
        L{getVersionString} includes the release candidate, if any.
        """
        self.assertEqual(
            getVersionString(Version("whatever", 8, 0, 0,
                                     release_candidate=1)),
            "whatever 8.0.0rc1")

    def test_getVersionStringWithDev(self):
        """
        L{getVersionString} includes the dev release, if any.
        """
        self.assertEqual(
            getVersionString(Version("whatever", 8, 0, 0,
                                     dev=1)),
            "whatever 8.0.0dev1")

    def test_getVersionStringWithDevAndRC(self):
        """
        L{getVersionString} includes the dev release and release candidate, if
        any.
        """
        self.assertEqual(
            getVersionString(Version("whatever", 8, 0, 0,
                                     release_candidate=2, dev=1)),
            "whatever 8.0.0rc2dev1")

    def test_baseWithNEXT(self):
        """
        The C{base} method returns just "NEXT" when NEXT is the major version.
        """
        self.assertEqual(Version("foo", "NEXT", 0, 0).base(), "NEXT")

    def test_base(self):
        """
        The C{base} method returns a very simple representation of the version.
        """
        self.assertEqual(Version("foo", 1, 0, 0).base(), "1.0.0")

    def test_baseWithPrerelease(self):
        """
        The base version includes 'rcX' for versions with prereleases.
        """
        self.assertEqual(Version("foo", 1, 0, 0, prerelease=8).base(),
                         "1.0.0rc8")

    def test_baseWithDev(self):
        """
        The base version includes 'devX' for versions with dev releases.
        """
        self.assertEqual(Version("foo", 1, 0, 0, dev=8).base(),
                         "1.0.0dev8")

    def test_baseWithReleaseCandidate(self):
        """
        The base version includes 'rcX' for versions with prereleases.
        """
        self.assertEqual(Version("foo", 1, 0, 0, release_candidate=8).base(),
                         "1.0.0rc8")

    def test_baseWithDevAndRC(self):
        """
        The base version includes 'rcXdevX' for versions with dev releases and
        a release candidate.
        """
        self.assertEqual(Version("foo", 1, 0, 0,
                                 release_candidate=2, dev=8).base(),
                         "1.0.0rc2dev8")
