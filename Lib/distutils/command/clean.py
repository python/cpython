"""distutils.command.clean

Implements the Distutils 'clean' command."""

# contributed by Bastian Kleineidam <calvin@cs.uni-sb.de>, added 2000-03-18

__revision__ = "$Id$"

import os
from distutils.core import Command
from distutils.util import remove_tree

class clean (Command):

    description = "clean up output of 'build' command"
    user_options = [
        ('build-base=', 'b',
         "base build directory (default: 'build.build-base')"),
        ('build-lib=', None,
         "build directory for all modules (default: 'build.build-lib')"),
        ('build-temp=', 't',
         "temporary build directory (default: 'build.build-temp')"),
        ('bdist-base=', None,
         "temporary directory for built distributions"),
        ('all', 'a',
         "remove all build output, not just temporary by-products")
    ]

    def initialize_options(self):
        self.build_base = None
        self.build_lib = None
        self.build_temp = None
        self.bdist_base = None
        self.all = None

    def finalize_options(self):
        if self.build_lib and not os.path.exists (self.build_lib):
            self.warn ("'%s' does not exist -- can't clean it" %
                       self.build_lib)
        if self.build_temp and not os.path.exists (self.build_temp):
            self.warn ("'%s' does not exist -- can't clean it" %
                       self.build_temp)

        self.set_undefined_options('build',
                                   ('build_base', 'build_base'),
                                   ('build_lib', 'build_lib'),
                                   ('build_temp', 'build_temp'))
        self.set_undefined_options('bdist',
                                   ('bdist_base', 'bdist_base'))

    def run(self):
        # remove the build/temp.<plat> directory (unless it's already
        # gone)
        if os.path.exists (self.build_temp):
            remove_tree (self.build_temp, self.verbose, self.dry_run)

        if self.all:
            # remove the module build directory (unless already gone)
            if os.path.exists (self.build_lib):
                remove_tree (self.build_lib, self.verbose, self.dry_run)
            # remove the temporary directory used for creating built
            # distributions (default "build/bdist") -- eg. type of
            # built distribution will have its own subdirectory under
            # "build/bdist", but they'll be taken care of by
            # 'remove_tree()'.
            if os.path.exists (self.bdist_base):
                remove_tree (self.bdist_base, self.verbose, self.dry_run)

        # just for the heck of it, try to remove the base build directory:
        # we might have emptied it right now, but if not we don't care
        if not self.dry_run:
            try:
                os.rmdir (self.build_base)
                self.announce ("removing '%s'" % self.build_base)
            except OSError:
                pass

# class clean
