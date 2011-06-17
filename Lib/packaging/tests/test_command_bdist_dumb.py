"""Tests for distutils.command.bdist_dumb."""

import os
import packaging.util

from packaging.dist import Distribution
from packaging.command.bdist_dumb import bdist_dumb
from packaging.tests import unittest, support
from packaging.tests.support import requires_zlib


class BuildDumbTestCase(support.TempdirManager,
                        support.LoggingCatcher,
                        unittest.TestCase):

    def setUp(self):
        super(BuildDumbTestCase, self).setUp()
        self.old_location = os.getcwd()

    def tearDown(self):
        os.chdir(self.old_location)
        packaging.util._path_created.clear()
        super(BuildDumbTestCase, self).tearDown()

    @requires_zlib
    def test_simple_built(self):

        # let's create a simple package
        tmp_dir = self.mkdtemp()
        pkg_dir = os.path.join(tmp_dir, 'foo')
        os.mkdir(pkg_dir)
        self.write_file((pkg_dir, 'foo.py'), '#')
        self.write_file((pkg_dir, 'MANIFEST.in'), 'include foo.py')
        self.write_file((pkg_dir, 'README'), '')

        dist = Distribution({'name': 'foo', 'version': '0.1',
                             'py_modules': ['foo'],
                             'url': 'xxx', 'author': 'xxx',
                             'author_email': 'xxx'})
        os.chdir(pkg_dir)
        cmd = bdist_dumb(dist)

        # so the output is the same no matter
        # what is the platform
        cmd.format = 'zip'

        cmd.ensure_finalized()
        cmd.run()

        # see what we have
        dist_created = os.listdir(os.path.join(pkg_dir, 'dist'))
        base = "%s.%s" % (dist.get_fullname(), cmd.plat_name)
        if os.name == 'os2':
            base = base.replace(':', '-')

        wanted = ['%s.zip' % base]
        self.assertEqual(dist_created, wanted)

        # now let's check what we have in the zip file
        # XXX to be done

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
    unittest.main(defaultTest='test_suite')
