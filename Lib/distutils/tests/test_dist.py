# -*- coding: utf8 -*-

"""Tests for distutils.dist."""
import os
import StringIO
import sys
import unittest
import warnings
import textwrap

from distutils.dist import Distribution, fix_help_options
from distutils.cmd import Command
import distutils.dist
from test.test_support import TESTFN, captured_stdout, run_unittest
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


class DistributionTestCase(support.TempdirManager,
                           support.LoggingSilencer,
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

    def test_debug_mode(self):
        with open(TESTFN, "w") as f:
            f.write("[global]\n")
            f.write("command_packages = foo.bar, splat")

        files = [TESTFN]
        sys.argv.append("build")

        with captured_stdout() as stdout:
            self.create_distribution(files)
        stdout.seek(0)
        self.assertEqual(stdout.read(), '')
        distutils.dist.DEBUG = True
        try:
            with captured_stdout() as stdout:
                self.create_distribution(files)
            stdout.seek(0)
            self.assertEqual(stdout.read(), '')
        finally:
            distutils.dist.DEBUG = False

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
        self.assertIsInstance(cmd, test_dist)
        self.assertEqual(cmd.sample_option, "sometext")

    def test_command_packages_configfile(self):
        sys.argv.append("build")
        self.addCleanup(os.unlink, TESTFN)
        f = open(TESTFN, "w")
        try:
            print >> f, "[global]"
            print >> f, "command_packages = foo.bar, splat"
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

    def test_write_pkg_file(self):
        # Check DistributionMetadata handling of Unicode fields
        tmp_dir = self.mkdtemp()
        my_file = os.path.join(tmp_dir, 'f')
        klass = Distribution

        dist = klass(attrs={'author': u'Mister Café',
                            'name': 'my.package',
                            'maintainer': u'Café Junior',
                            'description': u'Café torréfié',
                            'long_description': u'Héhéhé'})

        # let's make sure the file can be written
        # with Unicode fields. they are encoded with
        # PKG_INFO_ENCODING
        dist.metadata.write_pkg_file(open(my_file, 'w'))

        # regular ascii is of course always usable
        dist = klass(attrs={'author': 'Mister Cafe',
                            'name': 'my.package',
                            'maintainer': 'Cafe Junior',
                            'description': 'Cafe torrefie',
                            'long_description': 'Hehehe'})

        my_file2 = os.path.join(tmp_dir, 'f2')
        dist.metadata.write_pkg_file(open(my_file2, 'w'))

    def test_empty_options(self):
        # an empty options dictionary should not stay in the
        # list of attributes

        # catching warnings
        warns = []

        def _warn(msg):
            warns.append(msg)

        self.addCleanup(setattr, warnings, 'warn', warnings.warn)
        warnings.warn = _warn
        dist = Distribution(attrs={'author': 'xxx', 'name': 'xxx',
                                   'version': 'xxx', 'url': 'xxxx',
                                   'options': {}})

        self.assertEqual(len(warns), 0)
        self.assertNotIn('options', dir(dist))

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

    def test_find_config_files_disable(self):
        # Ticket #1180: Allow user to disable their home config file.
        temp_home = self.mkdtemp()
        if os.name == 'posix':
            user_filename = os.path.join(temp_home, ".pydistutils.cfg")
        else:
            user_filename = os.path.join(temp_home, "pydistutils.cfg")

        with open(user_filename, 'w') as f:
            f.write('[distutils]\n')

        def _expander(path):
            return temp_home

        old_expander = os.path.expanduser
        os.path.expanduser = _expander
        try:
            d = distutils.dist.Distribution()
            all_files = d.find_config_files()

            d = distutils.dist.Distribution(attrs={'script_args':
                                            ['--no-user-cfg']})
            files = d.find_config_files()
        finally:
            os.path.expanduser = old_expander

        # make sure --no-user-cfg disables the user cfg file
        self.assertEqual(len(all_files)-1, len(files))


class MetadataTestCase(support.TempdirManager, support.EnvironGuard,
                       unittest.TestCase):

    def setUp(self):
        super(MetadataTestCase, self).setUp()
        self.argv = sys.argv, sys.argv[:]

    def tearDown(self):
        sys.argv = self.argv[0]
        sys.argv[:] = self.argv[1]
        super(MetadataTestCase, self).tearDown()

    def test_classifier(self):
        attrs = {'name': 'Boa', 'version': '3.0',
                 'classifiers': ['Programming Language :: Python :: 3']}
        dist = Distribution(attrs)
        meta = self.format_metadata(dist)
        self.assertIn('Metadata-Version: 1.1', meta)

    def test_download_url(self):
        attrs = {'name': 'Boa', 'version': '3.0',
                 'download_url': 'http://example.org/boa'}
        dist = Distribution(attrs)
        meta = self.format_metadata(dist)
        self.assertIn('Metadata-Version: 1.1', meta)

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
        self.assertIn(long_desc, meta)

    def test_simple_metadata(self):
        attrs = {"name": "package",
                 "version": "1.0"}
        dist = Distribution(attrs)
        meta = self.format_metadata(dist)
        self.assertIn("Metadata-Version: 1.0", meta)
        self.assertNotIn("provides:", meta.lower())
        self.assertNotIn("requires:", meta.lower())
        self.assertNotIn("obsoletes:", meta.lower())

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
        self.assertIn("Metadata-Version: 1.1", meta)
        self.assertNotIn("requires:", meta.lower())
        self.assertNotIn("obsoletes:", meta.lower())

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
        self.assertIn("Metadata-Version: 1.1", meta)
        self.assertNotIn("provides:", meta.lower())
        self.assertIn("Requires: other", meta)
        self.assertIn("Requires: another (==1.0)", meta)
        self.assertNotIn("obsoletes:", meta.lower())

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
        self.assertIn("Metadata-Version: 1.1", meta)
        self.assertNotIn("provides:", meta.lower())
        self.assertNotIn("requires:", meta.lower())
        self.assertIn("Obsoletes: other", meta)
        self.assertIn("Obsoletes: another (<1.0)", meta)

    def test_obsoletes_illegal(self):
        self.assertRaises(ValueError, Distribution,
                          {"name": "package",
                           "version": "1.0",
                           "obsoletes": ["my.pkg (splat)"]})

    def format_metadata(self, dist):
        sio = StringIO.StringIO()
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
                self.assertIn(user_filename, files)

            # win32-style
            if sys.platform == 'win32':
                # home drive should be found
                os.environ['HOME'] = temp_dir
                files = dist.find_config_files()
                self.assertIn(user_filename, files,
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
        self.assertTrue(output)

    def test_read_metadata(self):
        attrs = {"name": "package",
                 "version": "1.0",
                 "long_description": "desc",
                 "description": "xxx",
                 "download_url": "http://example.com",
                 "keywords": ['one', 'two'],
                 "requires": ['foo']}

        dist = Distribution(attrs)
        metadata = dist.metadata

        # write it then reloads it
        PKG_INFO = StringIO.StringIO()
        metadata.write_pkg_file(PKG_INFO)
        PKG_INFO.seek(0)
        metadata.read_pkg_file(PKG_INFO)

        self.assertEqual(metadata.name, "package")
        self.assertEqual(metadata.version, "1.0")
        self.assertEqual(metadata.description, "xxx")
        self.assertEqual(metadata.download_url, 'http://example.com')
        self.assertEqual(metadata.keywords, ['one', 'two'])
        self.assertEqual(metadata.platforms, ['UNKNOWN'])
        self.assertEqual(metadata.obsoletes, None)
        self.assertEqual(metadata.requires, ['foo'])


def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(DistributionTestCase))
    suite.addTest(unittest.makeSuite(MetadataTestCase))
    return suite

if __name__ == "__main__":
    run_unittest(test_suite())
