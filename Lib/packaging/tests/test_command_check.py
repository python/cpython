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

        # clear warnings from the previous calls
        self.loghandler.flush()

        # and of course, no error when all metadata fields are present
        cmd = self._run(metadata, strict=True)
        self.assertEqual([], self.get_logs(logging.WARNING))

        # now a test with non-ASCII characters
        metadata = {'home_page': 'xxx', 'author': '\u00c9ric',
                    'author_email': 'xxx', 'name': 'xxx',
                    'version': '1.2',
                    'summary': 'Something about esszet \u00df',
                    'description': 'More things about esszet \u00df'}
        cmd = self._run(metadata)
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

        # clear warnings from the previous calls
        self.loghandler.flush()

        # now with correct version format again
        metadata['version'] = '4.2'
        cmd = self._run(metadata, strict=True)
        self.assertEqual([], self.get_logs(logging.WARNING))

    @unittest.skipUnless(_HAS_DOCUTILS, "requires docutils")
    def test_check_restructuredtext(self):
        # let's see if it detects broken rest in description
        broken_rest = 'title\n===\n\ntest'
        pkg_info, dist = self.create_dist(description=broken_rest)
        cmd = check(dist)
        cmd.check_restructuredtext()
        self.assertEqual(len(self.get_logs(logging.WARNING)), 1)
        # clear warnings from the previous call
        self.loghandler.flush()

        # let's see if we have an error with strict=1
        metadata = {'home_page': 'xxx', 'author': 'xxx',
                    'author_email': 'xxx',
                    'name': 'xxx', 'version': '1.2',
                    'description': broken_rest}
        self.assertRaises(PackagingSetupError, self._run, metadata,
                          strict=True, all=True)
        self.loghandler.flush()

        # and non-broken rest, including a non-ASCII character to test #12114
        dist = self.create_dist(description='title\n=====\n\ntest \u00df')[1]
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

    def test_warn(self):
        _, dist = self.create_dist()
        cmd = check(dist)
        self.assertEqual([], self.get_logs())
        cmd.warn('hello')
        self.assertEqual(['check: hello'], self.get_logs())
        cmd.warn('hello %s', 'world')
        self.assertEqual(['check: hello world'], self.get_logs())
        cmd.warn('hello %s %s', 'beautiful', 'world')
        self.assertEqual(['check: hello beautiful world'], self.get_logs())


def test_suite():
    return unittest.makeSuite(CheckTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
