"""distutils.command.build

Implements the Distutils 'build' command."""

# created 1999/03/08, Greg Ward

__revision__ = "$Id$"

import sys, os
from distutils.core import Command
from distutils.util import get_platform

class build (Command):

    description = "build everything needed to install"

    user_options = [
        ('build-base=', 'b',
         "base directory for build library"),
        ('build-purelib=', None,
         "build directory for platform-neutral distributions"),
        ('build-platlib=', None,
         "build directory for platform-specific distributions"),
        ('build-lib=', None,
         "build directory for all distribution (defaults to either " +
         "build-purelib or build-platlib"),
        ('build-temp=', 't',
         "temporary build directory"),
        ('compiler=', 'c',
         "specify the compiler type"),
        ('debug', 'g',
         "compile extensions and libraries with debugging information"),
        ('force', 'f',
         "forcibly build everything (ignore file timestamps)"),
        ]

    def initialize_options (self):
        self.build_base = 'build'
        # these are decided only after 'build_base' has its final value
        # (unless overridden by the user or client)
        self.build_purelib = None
        self.build_platlib = None
        self.build_lib = None
        self.build_temp = None
        self.compiler = None
        self.debug = None
        self.force = 0

    def finalize_options (self):

        # Need this to name platform-specific directories, but sys.platform
        # is not enough -- it only names the OS and version, not the
        # hardware architecture!
        self.plat = get_platform ()

        # 'build_purelib' and 'build_platlib' just default to 'lib' and
        # 'lib.<plat>' under the base build directory.  We only use one of
        # them for a given distribution, though --
        if self.build_purelib is None:
            self.build_purelib = os.path.join (self.build_base, 'lib')
        if self.build_platlib is None:
            self.build_platlib = os.path.join (self.build_base,
                                               'lib.' + self.plat)

        # 'build_lib' is the actual directory that we will use for this
        # particular module distribution -- if user didn't supply it, pick
        # one of 'build_purelib' or 'build_platlib'.
        if self.build_lib is None:
            if self.distribution.ext_modules:
                self.build_lib = self.build_platlib
            else:
                self.build_lib = self.build_purelib

        # 'build_temp' -- temporary directory for compiler turds,
        # "build/temp.<plat>"
        if self.build_temp is None:
            self.build_temp = os.path.join (self.build_base,
                                            'temp.' + self.plat)
    # finalize_options ()


    def run (self):

        # For now, "build" means "build_py" then "build_ext".  (Eventually
        # it should also build documentation.)

        # Invoke the 'build_py' command to "build" pure Python modules
        # (ie. copy 'em into the build tree)
        if self.distribution.has_pure_modules():
            self.run_peer ('build_py')

        # Build any standalone C libraries next -- they're most likely to
        # be needed by extension modules, so obviously have to be done
        # first!
        if self.distribution.has_c_libraries():
            self.run_peer ('build_clib')

        # And now 'build_ext' -- compile extension modules and put them
        # into the build tree
        if self.distribution.has_ext_modules():
            self.run_peer ('build_ext')

# class build
