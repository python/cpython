"""install_ext

Implement the Distutils "install_ext" command to install extension modules."""

# created 1999/09/12, Greg Ward

__rcsid__ = "$Id$"

from distutils.core import Command
from distutils.util import copy_tree

class InstallExt (Command):

    options = [('dir=', 'd', "directory to install to"),
               ('build-dir=','b', "build directory (where to install from)"),
              ]

    def set_default_options (self):
        # let the 'install' command dictate our installation directory
        self.dir = None
        self.build_dir = None

    def set_final_options (self):
        self.set_undefined_options ('install',
                                    ('build_platlib', 'build_dir'),
                                    ('install_site_platlib', 'dir'))

    def run (self):
        self.set_final_options ()

        # Dump the entire "build/platlib" directory (or whatever it really
        # is; "build/platlib" is the default) to the installation target
        # (eg. "/usr/local/lib/python1.5/site-packages").  Note that
        # putting files in the right package dir is already done when we
        # build.
        outfiles = self.copy_tree (self.build_dir, self.dir)

# class InstallExt
