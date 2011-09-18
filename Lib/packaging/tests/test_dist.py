"""Tests for packaging.dist."""
import os
import sys
import logging
import textwrap

import packaging.dist

from packaging.dist import Distribution
from packaging.command import set_command
from packaging.command.cmd import Command
from packaging.errors import PackagingModuleError, PackagingOptionError
from packaging.tests import captured_stdout
from packaging.tests import support, unittest
from packaging.tests.support import create_distribution
from test.support import unload


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

    restore_environ = ['HOME', 'PLAT']

    def setUp(self):
        super(DistributionTestCase, self).setUp()
        self.argv = sys.argv, sys.argv[:]
        del sys.argv[1:]

    def tearDown(self):
        sys.argv = self.argv[0]
        sys.argv[:] = self.argv[1]
        super(DistributionTestCase, self).tearDown()

    @unittest.skip('needs to be updated')
    def test_debug_mode(self):
        tmpdir = self.mkdtemp()
        setupcfg = os.path.join(tmpdir, 'setup.cfg')
        with open(setupcfg, "w") as f:
            f.write("[global]\n")
            f.write("command_packages = foo.bar, splat")

        files = [setupcfg]
        sys.argv.append("build")
        __, stdout = captured_stdout(create_distribution, files)
        self.assertEqual(stdout, '')
        # XXX debug mode does not exist anymore, test logging levels in this
        # test instead
        packaging.dist.DEBUG = True
        try:
            __, stdout = captured_stdout(create_distribution, files)
            self.assertEqual(stdout, '')
        finally:
            packaging.dist.DEBUG = False

    def test_bad_attr(self):
        Distribution(attrs={'author': 'xxx',
                            'name': 'xxx',
                            'version': '1.2',
                            'home-page': 'xxxx',
                            'badoptname': 'xxx'})
        logs = self.get_logs(logging.WARNING)
        self.assertEqual(len(logs), 1)
        self.assertIn('unknown argument', logs[0])

    def test_empty_options(self):
        # an empty options dictionary should not stay in the
        # list of attributes
        dist = Distribution(attrs={'author': 'xxx', 'name': 'xxx',
                                   'version': '1.2', 'home-page': 'xxxx',
                                   'options': {}})

        self.assertEqual([], self.get_logs(logging.WARNING))
        self.assertNotIn('options', dir(dist))

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
                                   'home-page': 'xxxx',
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

    def test_custom_pydistutils(self):
        # Bug #2166:  make sure pydistutils.cfg is found
        if os.name == 'posix':
            user_filename = ".pydistutils.cfg"
        else:
            user_filename = "pydistutils.cfg"

        temp_dir = self.mkdtemp()
        user_filename = os.path.join(temp_dir, user_filename)
        with open(user_filename, 'w') as f:
            f.write('.')

        dist = Distribution()

        os.environ['HOME'] = temp_dir
        files = dist.find_config_files()
        self.assertIn(user_filename, files)

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

        self.write_file(config_file, textwrap.dedent('''\
            [test_dist]
            pre-hook.test = %(modname)s.log_pre_call
            post-hook.test = %(modname)s.log_post_call'''
            % {'modname': module_name}))

        self.write_file(hooks_module, textwrap.dedent('''\
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
        self.addCleanup(unload, module_name)
        record = __import__(module_name).record

        cmd.run = lambda: record.append('run')
        cmd.finalize_options = lambda: record.append('finalize')
        d.run_command('test_dist')

        self.assertEqual(record, ['finalize',
                                  'pre-test_dist',
                                  'run',
                                  'post-test_dist'])

    def test_hooks_importable(self):
        temp_home = self.mkdtemp()
        config_file = os.path.join(temp_home, "config1.cfg")

        self.write_file(config_file, textwrap.dedent('''\
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

        self.write_file(config_file, textwrap.dedent('''\
            [test_dist]
            pre-hook.test = packaging.tests.test_dist.__doc__'''))

        set_command('packaging.tests.test_dist.test_dist')
        d = create_distribution([config_file])
        cmd = d.get_command_obj("test_dist")
        cmd.ensure_finalized()

        self.assertRaises(PackagingOptionError, d.run_command, 'test_dist')


def test_suite():
    return unittest.makeSuite(DistributionTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
