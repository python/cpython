"""Tests for packaging.dist."""
import os
import io
import sys
import logging
import textwrap
import packaging.dist

from packaging.dist import Distribution
from packaging.command import set_command
from packaging.command.cmd import Command
from packaging.errors import PackagingModuleError, PackagingOptionError
from packaging.tests import TESTFN, captured_stdout
from packaging.tests import support, unittest
from packaging.tests.support import create_distribution


class test_dist(Command):
    """Sample packaging extension command."""

    user_options = [
        ("sample-option=", "S", "help text"),
        ]

    def initialize_options(self):
        self.sample_option = None

    def finalize_options(self):
        pass


class DistributionTestCase(support.TempdirManager,
                           support.LoggingCatcher,
                           support.EnvironRestorer,
                           unittest.TestCase):

    restore_environ = ['HOME']

    def setUp(self):
        super(DistributionTestCase, self).setUp()
        self.argv = sys.argv, sys.argv[:]
        del sys.argv[1:]

    def tearDown(self):
        sys.argv = self.argv[0]
        sys.argv[:] = self.argv[1]
        super(DistributionTestCase, self).tearDown()

    def test_debug_mode(self):
        self.addCleanup(os.unlink, TESTFN)
        with open(TESTFN, "w") as f:
            f.write("[global]\n")
            f.write("command_packages = foo.bar, splat")

        files = [TESTFN]
        sys.argv.append("build")
        __, stdout = captured_stdout(create_distribution, files)
        self.assertEqual(stdout, '')
        packaging.dist.DEBUG = True
        try:
            __, stdout = captured_stdout(create_distribution, files)
            self.assertEqual(stdout, '')
        finally:
            packaging.dist.DEBUG = False

    def test_write_pkg_file(self):
        # Check Metadata handling of Unicode fields
        tmp_dir = self.mkdtemp()
        my_file = os.path.join(tmp_dir, 'f')
        cls = Distribution

        dist = cls(attrs={'author': 'Mister Café',
                          'name': 'my.package',
                          'maintainer': 'Café Junior',
                          'summary': 'Café torréfié',
                          'description': 'Héhéhé'})

        # let's make sure the file can be written
        # with Unicode fields. they are encoded with
        # PKG_INFO_ENCODING
        with open(my_file, 'w') as fp:
            dist.metadata.write_file(fp)

        # regular ascii is of course always usable
        dist = cls(attrs={'author': 'Mister Cafe',
                          'name': 'my.package',
                          'maintainer': 'Cafe Junior',
                          'summary': 'Cafe torrefie',
                          'description': 'Hehehe'})

        with open(my_file, 'w') as fp:
            dist.metadata.write_file(fp)

    def test_bad_attr(self):
        Distribution(attrs={'author': 'xxx',
                            'name': 'xxx',
                            'version': '1.2',
                            'url': 'xxxx',
                            'badoptname': 'xxx'})
        logs = self.get_logs(logging.WARNING)
        self.assertEqual(1, len(logs))
        self.assertIn('unknown argument', logs[0])

    def test_bad_version(self):
        Distribution(attrs={'author': 'xxx',
                            'name': 'xxx',
                            'version': 'xxx',
                            'url': 'xxxx'})
        logs = self.get_logs(logging.WARNING)
        self.assertEqual(1, len(logs))
        self.assertIn('not a valid version', logs[0])

    def test_empty_options(self):
        # an empty options dictionary should not stay in the
        # list of attributes
        Distribution(attrs={'author': 'xxx',
                            'name': 'xxx',
                            'version': '1.2',
                            'url': 'xxxx',
                            'options': {}})

        self.assertEqual([], self.get_logs(logging.WARNING))

    def test_non_empty_options(self):
        # TODO: how to actually use options is not documented except
        # for a few cryptic comments in dist.py.  If this is to stay
        # in the public API, it deserves some better documentation.

        # Here is an example of how it's used out there:
        # http://svn.pythonmac.org/py2app/py2app/trunk/doc/
        # index.html#specifying-customizations
        dist = Distribution(attrs={'author': 'xxx',
                                   'name': 'xxx',
                                   'version': 'xxx',
                                   'url': 'xxxx',
                                   'options': {'sdist': {'owner': 'root'}}})

        self.assertIn('owner', dist.get_option_dict('sdist'))

    def test_finalize_options(self):

        attrs = {'keywords': 'one,two',
                 'platform': 'one,two'}

        dist = Distribution(attrs=attrs)
        dist.finalize_options()

        # finalize_option splits platforms and keywords
        self.assertEqual(dist.metadata['platform'], ['one', 'two'])
        self.assertEqual(dist.metadata['keywords'], ['one', 'two'])

    def test_find_config_files_disable(self):
        # Bug #1180: Allow users to disable their own config file.
        temp_home = self.mkdtemp()
        if os.name == 'posix':
            user_filename = os.path.join(temp_home, ".pydistutils.cfg")
        else:
            user_filename = os.path.join(temp_home, "pydistutils.cfg")

        with open(user_filename, 'w') as f:
            f.write('[distutils2]\n')

        def _expander(path):
            return temp_home

        old_expander = os.path.expanduser
        os.path.expanduser = _expander
        try:
            d = packaging.dist.Distribution()
            all_files = d.find_config_files()

            d = packaging.dist.Distribution(attrs={'script_args':
                                                   ['--no-user-cfg']})
            files = d.find_config_files()
        finally:
            os.path.expanduser = old_expander

        # make sure --no-user-cfg disables the user cfg file
        self.assertEqual((len(all_files) - 1), len(files))

    def test_special_hooks_parsing(self):
        temp_home = self.mkdtemp()
        config_files = [os.path.join(temp_home, "config1.cfg"),
                        os.path.join(temp_home, "config2.cfg")]

        # Store two aliased hooks in config files
        self.write_file((temp_home, "config1.cfg"),
                        '[test_dist]\npre-hook.a = type')
        self.write_file((temp_home, "config2.cfg"),
                        '[test_dist]\npre-hook.b = type')

        set_command('packaging.tests.test_dist.test_dist')
        dist = create_distribution(config_files)
        cmd = dist.get_command_obj("test_dist")
        self.assertEqual(cmd.pre_hook, {"a": 'type', "b": 'type'})

    def test_hooks_get_run(self):
        temp_home = self.mkdtemp()
        module_name = os.path.split(temp_home)[-1]
        pyname = '%s.py' % module_name
        config_file = os.path.join(temp_home, "config1.cfg")
        hooks_module = os.path.join(temp_home, pyname)

        self.write_file(config_file, textwrap.dedent('''
            [test_dist]
            pre-hook.test = %(modname)s.log_pre_call
            post-hook.test = %(modname)s.log_post_call'''
            % {'modname': module_name}))

        self.write_file(hooks_module, textwrap.dedent('''
        record = []

        def log_pre_call(cmd):
            record.append('pre-%s' % cmd.get_command_name())

        def log_post_call(cmd):
            record.append('post-%s' % cmd.get_command_name())
        '''))

        set_command('packaging.tests.test_dist.test_dist')
        d = create_distribution([config_file])
        cmd = d.get_command_obj("test_dist")

        # prepare the call recorders
        sys.path.append(temp_home)
        self.addCleanup(sys.path.remove, temp_home)
        record = __import__(module_name).record

        old_run = cmd.run
        old_finalize = cmd.finalize_options
        cmd.run = lambda: record.append('run')
        cmd.finalize_options = lambda: record.append('finalize')
        try:
            d.run_command('test_dist')
        finally:
            cmd.run = old_run
            cmd.finalize_options = old_finalize

        self.assertEqual(record, ['finalize',
                                  'pre-test_dist',
                                  'run',
                                  'post-test_dist'])

    def test_hooks_importable(self):
        temp_home = self.mkdtemp()
        config_file = os.path.join(temp_home, "config1.cfg")

        self.write_file(config_file, textwrap.dedent('''
            [test_dist]
            pre-hook.test = nonexistent.dotted.name'''))

        set_command('packaging.tests.test_dist.test_dist')
        d = create_distribution([config_file])
        cmd = d.get_command_obj("test_dist")
        cmd.ensure_finalized()

        self.assertRaises(PackagingModuleError, d.run_command, 'test_dist')

    def test_hooks_callable(self):
        temp_home = self.mkdtemp()
        config_file = os.path.join(temp_home, "config1.cfg")

        self.write_file(config_file, textwrap.dedent('''
            [test_dist]
            pre-hook.test = packaging.tests.test_dist.__doc__'''))

        set_command('packaging.tests.test_dist.test_dist')
        d = create_distribution([config_file])
        cmd = d.get_command_obj("test_dist")
        cmd.ensure_finalized()

        self.assertRaises(PackagingOptionError, d.run_command, 'test_dist')


class MetadataTestCase(support.TempdirManager,
                       support.LoggingCatcher,
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
        self.assertIn("Metadata-Version: 1.0", meta)
        self.assertNotIn("provides:", meta.lower())
        self.assertNotIn("requires:", meta.lower())
        self.assertNotIn("obsoletes:", meta.lower())

    def test_provides_dist(self):
        attrs = {"name": "package",
                 "version": "1.0",
                 "provides_dist": ["package", "package.sub"]}
        dist = Distribution(attrs)
        self.assertEqual(dist.metadata['Provides-Dist'],
                         ["package", "package.sub"])
        meta = self.format_metadata(dist)
        self.assertIn("Metadata-Version: 1.2", meta)
        self.assertNotIn("requires:", meta.lower())
        self.assertNotIn("obsoletes:", meta.lower())

    def _test_provides_illegal(self):
        # XXX to do: check the versions
        self.assertRaises(ValueError, Distribution,
                          {"name": "package",
                           "version": "1.0",
                           "provides_dist": ["my.pkg (splat)"]})

    def test_requires_dist(self):
        attrs = {"name": "package",
                 "version": "1.0",
                 "requires_dist": ["other", "another (==1.0)"]}
        dist = Distribution(attrs)
        self.assertEqual(dist.metadata['Requires-Dist'],
                         ["other", "another (==1.0)"])
        meta = self.format_metadata(dist)
        self.assertIn("Metadata-Version: 1.2", meta)
        self.assertNotIn("provides:", meta.lower())
        self.assertIn("Requires-Dist: other", meta)
        self.assertIn("Requires-Dist: another (==1.0)", meta)
        self.assertNotIn("obsoletes:", meta.lower())

    def _test_requires_illegal(self):
        # XXX
        self.assertRaises(ValueError, Distribution,
                          {"name": "package",
                           "version": "1.0",
                           "requires": ["my.pkg (splat)"]})

    def test_obsoletes_dist(self):
        attrs = {"name": "package",
                 "version": "1.0",
                 "obsoletes_dist": ["other", "another (<1.0)"]}
        dist = Distribution(attrs)
        self.assertEqual(dist.metadata['Obsoletes-Dist'],
                         ["other", "another (<1.0)"])
        meta = self.format_metadata(dist)
        self.assertIn("Metadata-Version: 1.2", meta)
        self.assertNotIn("provides:", meta.lower())
        self.assertNotIn("requires:", meta.lower())
        self.assertIn("Obsoletes-Dist: other", meta)
        self.assertIn("Obsoletes-Dist: another (<1.0)", meta)

    def _test_obsoletes_illegal(self):
        # XXX
        self.assertRaises(ValueError, Distribution,
                          {"name": "package",
                           "version": "1.0",
                           "obsoletes": ["my.pkg (splat)"]})

    def format_metadata(self, dist):
        sio = io.StringIO()
        dist.metadata.write_file(sio)
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
        with open(user_filename, 'w') as f:
            f.write('.')

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
            self.assertIn(user_filename, files)

    def test_show_help(self):
        # smoke test, just makes sure some help is displayed
        dist = Distribution()
        sys.argv = []
        dist.help = True
        dist.script_name = 'setup.py'
        __, stdout = captured_stdout(dist.parse_command_line)
        output = [line for line in stdout.split('\n')
                  if line.strip() != '']
        self.assertGreater(len(output), 0)

    def test_description(self):
        desc = textwrap.dedent("""\
        example::
              We start here
            and continue here
          and end here.""")
        attrs = {"name": "package",
                 "version": "1.0",
                 "description": desc}

        dist = packaging.dist.Distribution(attrs)
        meta = self.format_metadata(dist)
        meta = meta.replace('\n' + 7 * ' ' + '|', '\n')
        self.assertIn(desc, meta)

    def test_read_metadata(self):
        attrs = {"name": "package",
                 "version": "1.0",
                 "description": "desc",
                 "summary": "xxx",
                 "download_url": "http://example.com",
                 "keywords": ['one', 'two'],
                 "requires_dist": ['foo']}

        dist = Distribution(attrs)
        metadata = dist.metadata

        # write it then reloads it
        PKG_INFO = io.StringIO()
        metadata.write_file(PKG_INFO)
        PKG_INFO.seek(0)

        metadata.read_file(PKG_INFO)
        self.assertEqual(metadata['name'], "package")
        self.assertEqual(metadata['version'], "1.0")
        self.assertEqual(metadata['summary'], "xxx")
        self.assertEqual(metadata['download_url'], 'http://example.com')
        self.assertEqual(metadata['keywords'], ['one', 'two'])
        self.assertEqual(metadata['platform'], [])
        self.assertEqual(metadata['obsoletes'], [])
        self.assertEqual(metadata['requires-dist'], ['foo'])


def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(DistributionTestCase))
    suite.addTest(unittest.makeSuite(MetadataTestCase))
    return suite

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
