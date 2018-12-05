"""Tests for distutils.command.bdist_dumb."""

import os
import sys
import zipfile
import unittest
from test.test_support import run_unittest

# zlib is not used here, but if it's not available
# test_simple_built will fail
try:
    import zlib
except ImportError:
    zlib = None

from distutils.core import Distribution
from distutils.command.bdist_dumb import bdist_dumb
from distutils.tests import support

SETUP_PY = """\
from distutils.core import setup
import foo

setup(name='foo', version='0.1', py_modules=['foo'],
      url='xxx', author='xxx', author_email='xxx')

"""

class BuildDumbTestCase(support.TempdirManager,
                        support.LoggingSilencer,
                        support.EnvironGuard,
                        unittest.TestCase):

    def setUp(self):
        super(BuildDumbTestCase, self).setUp()
        self.old_location = os.getcwd()
        self.old_sys_argv = sys.argv, sys.argv[:]

    def tearDown(self):
        os.chdir(self.old_location)
        sys.argv = self.old_sys_argv[0]
        sys.argv[:] = self.old_sys_argv[1]
        super(BuildDumbTestCase, self).tearDown()

    @unittest.skipUnless(zlib, "requires zlib")
    def test_simple_built(self):

        # let's create a simple package
        tmp_dir = self.mkdtemp()
        pkg_dir = os.path.join(tmp_dir, 'foo')
        os.mkdir(pkg_dir)
        self.write_file((pkg_dir, 'setup.py'), SETUP_PY)
        self.write_file((pkg_dir, 'foo.py'), '#')
        self.write_file((pkg_dir, 'MANIFEST.in'), 'include foo.py')
        self.write_file((pkg_dir, 'README'), '')

        dist = Distribution({'name': 'foo', 'version': '0.1',
                             'py_modules': ['foo'],
                             'url': 'xxx', 'author': 'xxx',
                             'author_email': 'xxx'})
        dist.script_name = 'setup.py'
        os.chdir(pkg_dir)

        sys.argv = ['setup.py']
        cmd = bdist_dumb(dist)

        # so the output is the same no matter
        # what is the platform
        cmd.format = 'zip'

        cmd.ensure_finalized()
        cmd.run()

        # see what we have
        dist_created = os.listdir(os.path.join(pkg_dir, 'dist'))
        base = "%s.%s.zip" % (dist.get_fullname(), cmd.plat_name)
        if os.name == 'os2':
            base = base.replace(':', '-')

        self.assertEqual(dist_created, [base])

        # now let's check what we have in the zip file
        fp = zipfile.ZipFile(os.path.join('dist', base))
        try:
            contents = fp.namelist()
        finally:
            fp.close()

        contents = sorted(filter(None, map(os.path.basename, contents)))
        wanted = ['foo-0.1-py%s.%s.egg-info' % sys.version_info[:2], 'foo.py']
        if not sys.dont_write_bytecode:
            wanted.append('foo.pyc')
        self.assertEqual(contents, sorted(wanted))

    def test_finalize_options(self):
        pkg_dir, dist = self.create_dist()
        os.chdir(pkg_dir)
        cmd = bdist_dumb(dist)
        self.assertEqual(cmd.bdist_dir, None)
        cmd.finalize_options()

        # bdist_dir is initialized to bdist_base/dumb if not set
        base = cmd.get_finalized_command('bdist').bdist_base
        self.assertEqual(cmd.bdist_dir, os.path.join(base, 'dumb'))

        # the format is set to a default value depending on the os.name
        default = cmd.default_format[os.name]
        self.assertEqual(cmd.format, default)

def test_suite():
    return unittest.makeSuite(BuildDumbTestCase)

if __name__ == '__main__':
    run_unittest(test_suite())
