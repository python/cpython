"""distutils.command.build

Implements the Distutils 'build' command."""

# created 1999/03/08, Greg Ward

__rcsid__ = "$Id$"

import os
from distutils.core import Command


class Build (Command):

    options = [('basedir=', 'b', "base directory for build library"),
               ('libdir=', 'l', "directory for platform-shared files"),
               ('platdir=', 'p', "directory for platform-specific files"),
              ]

    def set_default_options (self):
        self.basedir = 'build'
        # these are decided only after 'basedir' has its final value
        # (unless overridden by the user or client)
        self.libdir = None
        self.platdir = None

    def set_final_options (self):
        # 'libdir' and 'platdir' just default to 'lib' and 'plat' under
        # the base build directory
        if self.libdir is None:
            self.libdir = os.path.join (self.basedir, 'lib')
        if self.platdir is None:
            self.platdir = os.path.join (self.basedir, 'plat')


    def run (self):

        self.set_final_options ()

        # For now, "build" means "build_py" then "build_ext".  (Eventually
        # it should also build documentation.)

        # Invoke the 'build_py' command
        self.run_peer ('build_py')

        # And now 'build_ext'
        #self.run_peer ('build_ext')

# end class Build
