"""Tests for distutils.command.check."""

import logging
from packaging.command.check import check
from packaging.metadata import _HAS_DOCUTILS
from packaging.errors import PackagingSetupError, MetadataMissingError
from packaging.tests import unittest, support


class CheckTestCase(support.LoggingCatcher,
                    support.TempdirManager,
                    unittest.TestCase):

    def _run(self, metadata=None, **options):
        if metadata is None:
            metadata = {'name': 'xxx', 'version': '1.2'}
        pkg_info, dist = self.create_dist(**metadata)
        cmd = check(dist)
        cmd.initialize_options()
        for name, value in options.items():
            setattr(cmd, name, value)
        cmd.ensure_finalized()
        cmd.run()
        return cmd

    def test_check_metadata(self):
        # let's run the command with no metadata at all
        # by default, check is checking the metadata
        # should have some warnings
        cmd = self._run()
        # trick: using assertNotEqual with an empty list will give us a more
        # useful error message than assertGreater(.., 0) when the code change
        # and the test fails
        self.assertNotEqual([], self.get_logs(logging.WARNING))

        # now let's add the required fields
        # and run it again, to make sure we don't get
        # any warning anymore
        self.loghandler.flush()
        metadata = {'home_page': 'xxx', 'author': 'xxx',
                    'author_email': 'xxx',
                    'name': 'xxx', 'version': '4.2',
                    }
        cmd = self._run(metadata)
        self.assertEqual([], self.get_logs(logging.WARNING))

        # now with the strict mode, we should
        # get an error if there are missing metadata
        self.assertRaises(MetadataMissingError, self._run, {}, **{'strict': 1})
        self.assertRaises(PackagingSetupError, self._run,
            {'name': 'xxx', 'version': 'xxx'}, **{'strict': 1})

        # and of course, no error when all metadata fields are present
        self.loghandler.flush()
        cmd = self._run(metadata, strict=True)
        self.assertEqual([], self.get_logs(logging.WARNING))

    def test_check_metadata_1_2(self):
        # let's run the command with no metadata at all
        # by default, check is checking the metadata
        # should have some warnings
        cmd = self._run()
        self.assertNotEqual([], self.get_logs(logging.WARNING))

        # now let's add the required fields and run it again, to make sure we
        # don't get any warning anymore let's use requires_python as a marker
        # to enforce Metadata-Version 1.2
        metadata = {'home_page': 'xxx', 'author': 'xxx',
                    'author_email': 'xxx',
                    'name': 'xxx', 'version': '4.2',
                    'requires_python': '2.4',
                    }
        self.loghandler.flush()
        cmd = self._run(metadata)
        self.assertEqual([], self.get_logs(logging.WARNING))

        # now with the strict mode, we should
        # get an error if there are missing metadata
        self.assertRaises(MetadataMissingError, self._run, {}, **{'strict': 1})
        self.assertRaises(PackagingSetupError, self._run,
            {'name': 'xxx', 'version': 'xxx'}, **{'strict': 1})

        # complain about version format
        metadata['version'] = 'xxx'
        self.assertRaises(PackagingSetupError, self._run, metadata,
            **{'strict': 1})

        # now with correct version format again
        metadata['version'] = '4.2'
        self.loghandler.flush()
        cmd = self._run(metadata, strict=True)
        self.assertEqual([], self.get_logs(logging.WARNING))

    @unittest.skipUnless(_HAS_DOCUTILS, "requires docutils")
    def test_check_restructuredtext(self):
        # let's see if it detects broken rest in long_description
        broken_rest = 'title\n===\n\ntest'
        pkg_info, dist = self.create_dist(description=broken_rest)
        cmd = check(dist)
        cmd.check_restructuredtext()
        self.assertEqual(len(self.get_logs(logging.WARNING)), 1)

        self.loghandler.flush()
        pkg_info, dist = self.create_dist(description='title\n=====\n\ntest')
        cmd = check(dist)
        cmd.check_restructuredtext()
        self.assertEqual([], self.get_logs(logging.WARNING))

    def test_check_all(self):
        self.assertRaises(PackagingSetupError, self._run,
                          {'name': 'xxx', 'version': 'xxx'}, **{'strict': 1,
                                 'all': 1})
        self.assertRaises(MetadataMissingError, self._run,
                          {}, **{'strict': 1,
                                 'all': 1})

    def test_check_hooks(self):
        pkg_info, dist = self.create_dist()
        dist.command_options['install_dist'] = {
            'pre_hook': ('file', {"a": 'some.nonextistant.hook.ghrrraarrhll'}),
        }
        cmd = check(dist)
        cmd.check_hooks_resolvable()
        self.assertEqual(len(self.get_logs(logging.WARNING)), 1)


def test_suite():
    return unittest.makeSuite(CheckTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
