import importlib.util as util
import sys
import unittest


class ImportModuleReleaseTests(unittest.TestCase):
    """
    Test release compatibility issues relating to importlib
    """
    def test_magic_number(self):
        """
        Each python minor release should generally have a MAGIC_NUMBER
        that does not change once the release reaches candidate status.

        Once a release reaches candidate status, an entry should be
        added to EXPECTED_MAGIC_NUMBERS. This test will then check that
        the actual MAGIC_NUMBER matches the expected value for the
        release.

        In exceptional cases, it may be required to change the MAGIC_NUMBER
        for a maintenance release. In this case the change should be
        discussed in dev-python. If a change is required, community
        stakeholders such as OS package maintainers must be notified
        in advance. Such exceptional releases will then require an
        adjustment to this test case.
        """
        if sys.version_info.releaselevel not in ('final', 'candidate'):
            return

        release = (sys.version_info.major, sys.version_info.minor)

        msg = (
            "Candidate and final releases require an entry in "
            "importlib.util.EXPECTED_MAGIC_NUMBERS. Set the expected "
            "magic number to the current MAGIC_NUMBER to continue "
            "with the release."
        )
        self.assertIn(release, util.EXPECTED_MAGIC_NUMBERS, msg=msg)
        expected = util.EXPECTED_MAGIC_NUMBERS[release]
        actual = int.from_bytes(util.MAGIC_NUMBER[:2], 'little')

        # Adjust for exceptional releases
        if release == (3, 5):
            expected = 3351 # Changed in 3.5.3 issue 27286

        msg = (
            "The MAGIC_NUMBER {0} does not match the expected value "
            "{1} for release {2}. Changing the MAGIC_NUMBER for a "
            "maintenance release requires discussion in dev-python and "
            "notification of community stakeholders."
            .format(actual, expected, release)
        )
        self.assertEqual(expected, actual, msg)


if __name__ == '__main__':
    unittest.main()
