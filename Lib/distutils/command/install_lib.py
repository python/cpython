# created 1999/03/13, Greg Ward

__rcsid__ = "$Id$"

import sys
from distutils.core import Command
from distutils.util import copy_tree

class InstallPy (Command):

    options = [('dir=', 'd', "directory to install to"),
               ('build-dir=' 'b', "build directory (where to install from)")]

    def set_default_options (self):
        # let the 'install' command dictate our installation directory
        self.dir = None
        self.build_dir = None

    def set_final_options (self):
        # If we don't have a 'dir' value, we'll have to ask the 'install'
        # command for one.  (This usually means the user ran 'install_py'
        # directly, rather than going through 'install' -- so in reality,
        # 'find_command_obj()' will create an 'install' command object,
        # which we then query.

        self.set_undefined_options ('install',
                                    ('build_lib', 'build_dir'),
                                    ('install_site_lib', 'dir'))

    def run (self):

        self.set_final_options ()

        # Dump entire contents of the build directory to the installation
        # directory (that's the beauty of having a build directory!)
        self.copy_tree (self.build_dir, self.dir)
                   
    # run ()

# class InstallPy
