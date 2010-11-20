"""Tests for distutils.dist."""
import os
import io
import sys
import unittest
import warnings
import textwrap

from distutils.dist import Distribution, fix_help_options
from distutils.cmd import Command

from test.support import TESTFN, captured_stdout, run_unittest
from distutils.tests import support


class test_dist(Command):
    """Sample distutils extension command."""

    user_options = [
        ("sample-option=", "S", "help text"),
        ]

    def initialize_options(self):
        self.sample_option = None


class TestDistribution(Distribution):
    """Distribution subclasses that avoids the default search for
    configuration files.

    The ._config_files attribute must be set before
    .parse_config_files() is called.
    """

    def find_config_files(self):
        return self._config_files


class DistributionTestCase(support.LoggingSilencer,
                           support.EnvironGuard,
                           unittest.TestCase):

    def setUp(self):
        super(DistributionTestCase, self).setUp()
        self.argv = sys.argv, sys.argv[:]
        del sys.argv[1:]

    def tearDown(self):
        sys.argv = self.argv[0]
        sys.argv[:] = self.argv[1]
        super(DistributionTestCase, self).tearDown()

    def create_distribution(self, configfiles=()):
        d = TestDistribution()
        d._config_files = configfiles
        d.parse_config_files()
        d.parse_command_line()
        return d

    def test_command_packages_unspecified(self):
        sys.argv.append("build")
        d = self.create_distribution()
        self.assertEqual(d.get_command_packages(), ["distutils.command"])

    def test_command_packages_cmdline(self):
        from distutils.tests.test_dist import test_dist
        sys.argv.extend(["--command-packages",
                         "foo.bar,distutils.tests",
                         "test_dist",
                         "-Ssometext",
                         ])
        d = self.create_distribution()
        # let's actually try to load our test command:
        self.assertEqual(d.get_command_packages(),
                         ["distutils.command", "foo.bar", "distutils.tests"])
        cmd = d.get_command_obj("test_dist")
        self.assertTrue(isinstance(cmd, test_dist))
        self.assertEqual(cmd.sample_option, "sometext")

    def test_command_packages_configfile(self):
        sys.argv.append("build")
        self.addCleanup(os.unlink, TESTFN)
        f = open(TESTFN, "w")
        try:
            print("[global]", file=f)
            print("command_packages = foo.bar, splat", file=f)
        finally:
            f.close()

        d = self.create_distribution([TESTFN])
        self.assertEqual(d.get_command_packages(),
                         ["distutils.command", "foo.bar", "splat"])

        # ensure command line overrides config:
        sys.argv[1:] = ["--command-packages", "spork", "build"]
        d = self.create_distribution([TESTFN])
        self.assertEqual(d.get_command_packages(),
                         ["distutils.command", "spork"])

        # Setting --command-packages to '' should cause the default to
        # be used even if a config file specified something else:
        sys.argv[1:] = ["--command-packages", "", "build"]
        d = self.create_distribution([TESTFN])
        self.assertEqual(d.get_command_packages(), ["distutils.command"])

    def test_empty_options(self):
        # an empty options dictionary should not stay in the
        # list of attributes
        klass = Distribution

        # catching warnings
        warns = []
        def _warn(msg):
            warns.append(msg)

        old_warn = warnings.warn
        warnings.warn = _warn
        try:
            dist = klass(attrs={'author': 'xxx',
                                'name': 'xxx',
                                'version': 'xxx',
                                'url': 'xxxx',
                                'options': {}})
        finally:
            warnings.warn = old_warn

        self.assertEqual(len(warns), 0)

    def test_finalize_options(self):

        attrs = {'keywords': 'one,two',
                 'platforms': 'one,two'}

        dist = Distribution(attrs=attrs)
        dist.finalize_options()

        # finalize_option splits platforms and keywords
        self.assertEqual(dist.metadata.platforms, ['one', 'two'])
        self.assertEqual(dist.metadata.keywords, ['one', 'two'])

    def test_get_command_packages(self):
        dist = Distribution()
        self.assertEqual(dist.command_packages, None)
        cmds = dist.get_command_packages()
        self.assertEqual(cmds, ['distutils.command'])
        self.assertEqual(dist.command_packages,
                         ['distutils.command'])

        dist.command_packages = 'one,two'
        cmds = dist.get_command_packages()
        self.assertEqual(cmds, ['distutils.command', 'one', 'two'])


    def test_announce(self):
        # make sure the level is known
        dist = Distribution()
        args = ('ok',)
        kwargs = {'level': 'ok2'}
        self.assertRaises(ValueError, dist.announce, args, kwargs)

class MetadataTestCase(support.TempdirManager, support.EnvironGuard,
                       unittest.TestCase):

    def setUp(self):
        super(MetadataTestCase, self).setUp()
        self.argv = sys.argv, sys.argv[:]

    def tearDown(self):
        sys.argv = self.argv[0]
        sys.argv[:] = self.argv[1]
        super(MetadataTestCase, self).tearDown()

    def test_simple_metadata(self):
        attrs = {"name": "package",
                 "version": "1.0"}
        dist = Distribution(attrs)
        meta = self.format_metadata(dist)
        self.assertTrue("Metadata-Version: 1.0" in meta)
        self.assertTrue("provides:" not in meta.lower())
        self.assertTrue("requires:" not in meta.lower())
        self.assertTrue("obsoletes:" not in meta.lower())

    def test_provides(self):
        attrs = {"name": "package",
                 "version": "1.0",
                 "provides": ["package", "package.sub"]}
        dist = Distribution(attrs)
        self.assertEqual(dist.metadata.get_provides(),
                         ["package", "package.sub"])
        self.assertEqual(dist.get_provides(),
                         ["package", "package.sub"])
        meta = self.format_metadata(dist)
        self.assertTrue("Metadata-Version: 1.1" in meta)
        self.assertTrue("requires:" not in meta.lower())
        self.assertTrue("obsoletes:" not in meta.lower())

    def test_provides_illegal(self):
        self.assertRaises(ValueError, Distribution,
                          {"name": "package",
                           "version": "1.0",
                           "provides": ["my.pkg (splat)"]})

    def test_requires(self):
        attrs = {"name": "package",
                 "version": "1.0",
                 "requires": ["other", "another (==1.0)"]}
        dist = Distribution(attrs)
        self.assertEqual(dist.metadata.get_requires(),
                         ["other", "another (==1.0)"])
        self.assertEqual(dist.get_requires(),
                         ["other", "another (==1.0)"])
        meta = self.format_metadata(dist)
        self.assertTrue("Metadata-Version: 1.1" in meta)
        self.assertTrue("provides:" not in meta.lower())
        self.assertTrue("Requires: other" in meta)
        self.assertTrue("Requires: another (==1.0)" in meta)
        self.assertTrue("obsoletes:" not in meta.lower())

    def test_requires_illegal(self):
        self.assertRaises(ValueError, Distribution,
                          {"name": "package",
                           "version": "1.0",
                           "requires": ["my.pkg (splat)"]})

    def test_obsoletes(self):
        attrs = {"name": "package",
                 "version": "1.0",
                 "obsoletes": ["other", "another (<1.0)"]}
        dist = Distribution(attrs)
        self.assertEqual(dist.metadata.get_obsoletes(),
                         ["other", "another (<1.0)"])
        self.assertEqual(dist.get_obsoletes(),
                         ["other", "another (<1.0)"])
        meta = self.format_metadata(dist)
        self.assertTrue("Metadata-Version: 1.1" in meta)
        self.assertTrue("provides:" not in meta.lower())
        self.assertTrue("requires:" not in meta.lower())
        self.assertTrue("Obsoletes: other" in meta)
        self.assertTrue("Obsoletes: another (<1.0)" in meta)

    def test_obsoletes_illegal(self):
        self.assertRaises(ValueError, Distribution,
                          {"name": "package",
                           "version": "1.0",
                           "obsoletes": ["my.pkg (splat)"]})

    def format_metadata(self, dist):
        sio = io.StringIO()
        dist.metadata.write_pkg_file(sio)
        return sio.getvalue()

    def test_custom_pydistutils(self):
        # fixes #2166
        # make sure pydistutils.cfg is found
        if os.name == 'posix':
            user_filename = ".pydistutils.cfg"
        else:
            user_filename = "pydistutils.cfg"

        temp_dir = self.mkdtemp()
        user_filename = os.path.join(temp_dir, user_filename)
        f = open(user_filename, 'w')
        try:
            f.write('.')
        finally:
            f.close()

        try:
            dist = Distribution()

            # linux-style
            if sys.platform in ('linux', 'darwin'):
                os.environ['HOME'] = temp_dir
                files = dist.find_config_files()
                self.assertTrue(user_filename in files)

            # win32-style
            if sys.platform == 'win32':
                # home drive should be found
                os.environ['HOME'] = temp_dir
                files = dist.find_config_files()
                self.assertTrue(user_filename in files,
                             '%r not found in %r' % (user_filename, files))
        finally:
            os.remove(user_filename)

    def test_fix_help_options(self):
        help_tuples = [('a', 'b', 'c', 'd'), (1, 2, 3, 4)]
        fancy_options = fix_help_options(help_tuples)
        self.assertEqual(fancy_options[0], ('a', 'b', 'c'))
        self.assertEqual(fancy_options[1], (1, 2, 3))

    def test_show_help(self):
        # smoke test, just makes sure some help is displayed
        dist = Distribution()
        sys.argv = []
        dist.help = 1
        dist.script_name = 'setup.py'
        with captured_stdout() as s:
            dist.parse_command_line()

        output = [line for line in s.getvalue().split('\n')
                  if line.strip() != '']
        self.assertTrue(len(output) > 0)

    def test_long_description(self):
        long_desc = textwrap.dedent("""\
        example::
              We start here
            and continue here
          and end here.""")
        attrs = {"name": "package",
                 "version": "1.0",
                 "long_description": long_desc}

        dist = Distribution(attrs)
        meta = self.format_metadata(dist)
        meta = meta.replace('\n' + 8 * ' ', '\n')
        self.assertTrue(long_desc in meta)

def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(DistributionTestCase))
    suite.addTest(unittest.makeSuite(MetadataTestCase))
    return suite

if __name__ == "__main__":
    run_unittest(test_suite())
