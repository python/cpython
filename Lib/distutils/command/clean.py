"""distutils.command.clean

Implements the Distutils 'clean' command."""

# contributed by Bastian Kleineidam <calvin@cs.uni-sb.de>, added 2000-03-18

__revision__ = "$Id$"

import os
from distutils.core import Command
from distutils.util import remove_tree

class clean (Command):

    description = "clean files we built"
    user_options = [
        ('build-base=', 'b', "base directory for build library"),
        ('build-lib=', None,
         "build directory for all distribution (defaults to either " +
         "build-purelib or build-platlib"),
        ('build-temp=', 't', "temporary build directory"),
        ('all', 'a',
         "remove all build output, not just temporary by-products")
    ]

    def initialize_options(self):
        self.build_base = None
        self.build_lib = None
        self.build_temp = None
        self.all = None

    def finalize_options(self):
        self.set_undefined_options('build',
	    ('build_base', 'build_base'),
	    ('build_lib', 'build_lib'),
	    ('build_temp', 'build_temp'))

    def run(self):
        # remove the build/temp.<plat> directory
        remove_tree (self.build_temp, self.verbose, self.dry_run)

        if self.all:
            # remove the build/lib resp. build/platlib directory
            remove_tree (self.build_lib, self.verbose, self.dry_run)
