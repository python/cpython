"""distutils.command.bdist_dumb

Implements the Distutils 'bdist_dumb' command (create a "dumb" built
distribution -- i.e., just an archive to be unpacked under $prefix or
$exec_prefix)."""

# created 2000/03/29, Greg Ward

__revision__ = "$Id$"

import os
from distutils.core import Command
from distutils.util import get_platform, create_tree, remove_tree
from distutils.errors import *

class bdist_dumb (Command):

    description = "create a \"dumb\" built distribution"

    user_options = [('bdist-dir=', 'd',
                     "temporary directory for creating the distribution"),
                    ('format=', 'f',
                     "archive format to create (tar, ztar, gztar, zip)"),
                    ('keep-tree', 'k',
                     "keep the pseudo-installation tree around after " +
                     "creating the distribution archive"),
                   ]

    default_format = { 'posix': 'gztar',
                       'nt': 'zip', }


    def initialize_options (self):
        self.bdist_dir = None
        self.format = None
        self.keep_tree = 0

    # initialize_options()


    def finalize_options (self):
        if self.bdist_dir is None:
            bdist_base = self.get_finalized_command('bdist').bdist_base
            self.bdist_dir = os.path.join(bdist_base, 'dumb')

        if self.format is None:
            try:
                self.format = self.default_format[os.name]
            except KeyError:
                raise DistutilsPlatformError, \
                      ("don't know how to create dumb built distributions " +
                       "on platform %s") % os.name

    # finalize_options()


    def run (self):

        self.run_command ('build')

        # XXX don't use 'self.get_finalized_command()', because it always runs
        # 'ensure_finalized()' on the command object; we explictly want a
        # command object that has *not* been finalized, so we can set
        # options on it!  (The option we set, 'root', is so that we can do
        # a proper "fake install" using this install command object.)
        install = self.distribution.get_command_obj('install')
        install.root = self.bdist_dir

        self.announce ("installing to %s" % self.bdist_dir)
        install.ensure_finalized()
        install.run()

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        archive_basename = "%s.%s" % (self.distribution.get_fullname(),
                                      get_platform())
        print "self.bdist_dir = %s" % self.bdist_dir
        print "self.format = %s" % self.format
        self.make_archive (archive_basename, self.format,
                           root_dir=self.bdist_dir)

        if not self.keep_tree:
            remove_tree (self.bdist_dir, self.verbose, self.dry_run)

    # run()

# class bdist_dumb
