"""distutils.command.build

Implements the Distutils 'build' command."""

# created 1999/03/08, Greg Ward

__rcsid__ = "$Id$"

import os
from distutils.core import Command


class Build (Command):

    description = "build everything needed to install"

    options = [('build-base=', 'b',
                "base directory for build library"),
               ('build-lib=', 'l',
                "directory for platform-shared files"),
               ('build-platlib=', 'p',
                "directory for platform-specific files"),
              ]

    def set_default_options (self):
        self.build_base = 'build'
        # these are decided only after 'build_base' has its final value
        # (unless overridden by the user or client)
        self.build_lib = None
        self.build_platlib = None

    def set_final_options (self):
        # 'build_lib' and 'build_platlib' just default to 'lib' and
        # 'platlib' under the base build directory
        if self.build_lib is None:
            self.build_lib = os.path.join (self.build_base, 'lib')
        if self.build_platlib is None:
            self.build_platlib = os.path.join (self.build_base, 'platlib')


    def run (self):

        # For now, "build" means "build_py" then "build_ext".  (Eventually
        # it should also build documentation.)

        # Invoke the 'build_py' command to "build" pure Python modules
        # (ie. copy 'em into the build tree)
        if self.distribution.packages or self.distribution.py_modules:
            self.run_peer ('build_py')

        # And now 'build_ext' -- compile extension modules and put them
        # into the build tree
        if self.distribution.ext_modules:
            self.run_peer ('build_ext')

# end class Build
