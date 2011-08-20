"""Tests for ``packaging.command.install_distinfo``. """

import os
import csv
import hashlib
import sysconfig

from packaging.command.install_distinfo import install_distinfo
from packaging.command.cmd import Command
from packaging.compiler.extension import Extension
from packaging.metadata import Metadata
from packaging.tests import unittest, support


class DummyInstallCmd(Command):

    def __init__(self, dist=None):
        self.outputs = []
        self.distribution = dist

    def __getattr__(self, name):
        return None

    def ensure_finalized(self):
        pass

    def get_outputs(self):
        return (self.outputs +
                self.get_finalized_command('install_distinfo').get_outputs())


class InstallDistinfoTestCase(support.TempdirManager,
                              support.LoggingCatcher,
                              unittest.TestCase):

    checkLists = lambda self, x, y: self.assertListEqual(sorted(x), sorted(y))

    def test_empty_install(self):
        pkg_dir, dist = self.create_dist(name='foo',
                                         version='1.0')
        install_dir = self.mkdtemp()

        install = DummyInstallCmd(dist)
        dist.command_obj['install_dist'] = install

        cmd = install_distinfo(dist)
        dist.command_obj['install_distinfo'] = cmd

        cmd.distinfo_dir = install_dir
        cmd.ensure_finalized()
        cmd.run()

        self.checkLists(os.listdir(install_dir), ['foo-1.0.dist-info'])

        dist_info = os.path.join(install_dir, 'foo-1.0.dist-info')
        self.checkLists(os.listdir(dist_info),
                        ['METADATA', 'RECORD', 'REQUESTED', 'INSTALLER'])
        with open(os.path.join(dist_info, 'INSTALLER')) as fp:
            self.assertEqual(fp.read(), 'distutils')
        with open(os.path.join(dist_info, 'REQUESTED')) as fp:
            self.assertEqual(fp.read(), '')
        meta_path = os.path.join(dist_info, 'METADATA')
        self.assertTrue(Metadata(path=meta_path).check())

    def test_installer(self):
        pkg_dir, dist = self.create_dist(name='foo',
                                         version='1.0')
        install_dir = self.mkdtemp()

        install = DummyInstallCmd(dist)
        dist.command_obj['install_dist'] = install

        cmd = install_distinfo(dist)
        dist.command_obj['install_distinfo'] = cmd

        cmd.distinfo_dir = install_dir
        cmd.installer = 'bacon-python'
        cmd.ensure_finalized()
        cmd.run()

        dist_info = os.path.join(install_dir, 'foo-1.0.dist-info')
        with open(os.path.join(dist_info, 'INSTALLER')) as fp:
            self.assertEqual(fp.read(), 'bacon-python')

    def test_requested(self):
        pkg_dir, dist = self.create_dist(name='foo',
                                         version='1.0')
        install_dir = self.mkdtemp()

        install = DummyInstallCmd(dist)
        dist.command_obj['install_dist'] = install

        cmd = install_distinfo(dist)
        dist.command_obj['install_distinfo'] = cmd

        cmd.distinfo_dir = install_dir
        cmd.requested = False
        cmd.ensure_finalized()
        cmd.run()

        dist_info = os.path.join(install_dir, 'foo-1.0.dist-info')
        self.checkLists(os.listdir(dist_info),
                        ['METADATA', 'RECORD', 'INSTALLER'])

    def test_no_record(self):
        pkg_dir, dist = self.create_dist(name='foo',
                                         version='1.0')
        install_dir = self.mkdtemp()

        install = DummyInstallCmd(dist)
        dist.command_obj['install_dist'] = install

        cmd = install_distinfo(dist)
        dist.command_obj['install_distinfo'] = cmd

        cmd.distinfo_dir = install_dir
        cmd.no_record = True
        cmd.ensure_finalized()
        cmd.run()

        dist_info = os.path.join(install_dir, 'foo-1.0.dist-info')
        self.checkLists(os.listdir(dist_info),
                        ['METADATA', 'REQUESTED', 'INSTALLER'])

    def test_record_basic(self):
        install_dir = self.mkdtemp()
        modules_dest = os.path.join(install_dir, 'lib')
        scripts_dest = os.path.join(install_dir, 'bin')
        project_dir, dist = self.create_dist(
            name='Spamlib', version='0.1',
            py_modules=['spam'], scripts=['spamd'],
            ext_modules=[Extension('_speedspam', ['_speedspam.c'])])

        # using a real install_dist command is too painful, so we use a mock
        # class that's only a holder for options to be used by install_distinfo
        # and we create placeholder files manually instead of using build_*.
        # the install_* commands will still be consulted by install_distinfo.
        os.chdir(project_dir)
        self.write_file('spam', '# Python module')
        self.write_file('spamd', '# Python script')
        extmod = '_speedspam%s' % sysconfig.get_config_var('SO')
        self.write_file(extmod, '')

        install = DummyInstallCmd(dist)
        install.outputs = ['spam', 'spamd', extmod]
        install.install_lib = modules_dest
        install.install_scripts = scripts_dest
        dist.command_obj['install_dist'] = install

        cmd = install_distinfo(dist)
        cmd.ensure_finalized()
        dist.command_obj['install_distinfo'] = cmd
        cmd.run()

        record = os.path.join(modules_dest, 'Spamlib-0.1.dist-info', 'RECORD')
        with open(record, encoding='utf-8') as fp:
            content = fp.read()

        found = []
        for line in content.splitlines():
            filename, checksum, size = line.split(',')
            filename = os.path.basename(filename)
            found.append((filename, checksum, size))

        expected = [
            ('spam', '6ab2f288ef2545868effe68757448b45', '15'),
            ('spamd','d13e6156ce78919a981e424b2fdcd974', '15'),
            (extmod, 'd41d8cd98f00b204e9800998ecf8427e', '0'),
            ('METADATA', '846de67e49c3b92c81fb1ebd7bc07046', '172'),
            ('INSTALLER', '44e3fde05f3f537ed85831969acf396d', '9'),
            ('REQUESTED', 'd41d8cd98f00b204e9800998ecf8427e', '0'),
            ('RECORD', '', ''),
        ]
        self.assertEqual(found, expected)

    def test_record(self):
        pkg_dir, dist = self.create_dist(name='foo',
                                         version='1.0')
        install_dir = self.mkdtemp()

        install = DummyInstallCmd(dist)
        dist.command_obj['install_dist'] = install

        fake_dists = os.path.join(os.path.dirname(__file__), 'fake_dists')
        fake_dists = os.path.realpath(fake_dists)

        # for testing, we simply add all files from _backport's fake_dists
        dirs = []
        for dir in os.listdir(fake_dists):
            full_path = os.path.join(fake_dists, dir)
            if (not dir.endswith('.egg') or dir.endswith('.egg-info') or
                dir.endswith('.dist-info')) and os.path.isdir(full_path):
                dirs.append(full_path)

        for dir in dirs:
            for path, subdirs, files in os.walk(dir):
                install.outputs += [os.path.join(path, f) for f in files]
                install.outputs += [os.path.join('path', f + 'c')
                                    for f in files if f.endswith('.py')]

        cmd = install_distinfo(dist)
        dist.command_obj['install_distinfo'] = cmd

        cmd.distinfo_dir = install_dir
        cmd.ensure_finalized()
        cmd.run()

        dist_info = os.path.join(install_dir, 'foo-1.0.dist-info')

        expected = []
        for f in install.get_outputs():
            if (f.endswith(('.pyc', '.pyo')) or f == os.path.join(
                install_dir, 'foo-1.0.dist-info', 'RECORD')):
                expected.append([f, '', ''])
            else:
                size = os.path.getsize(f)
                md5 = hashlib.md5()
                with open(f, 'rb') as fp:
                    md5.update(fp.read())
                hash = md5.hexdigest()
                expected.append([f, hash, str(size)])

        parsed = []
        with open(os.path.join(dist_info, 'RECORD'), 'r') as f:
            reader = csv.reader(f, delimiter=',',
                                   lineterminator=os.linesep,
                                   quotechar='"')
            parsed = list(reader)

        self.maxDiff = None
        self.checkLists(parsed, expected)


def test_suite():
    return unittest.makeSuite(InstallDistinfoTestCase)


if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
