"""distutils.command.install_scripts

Implements the Distutils 'install_scripts' command, for installing
Python scripts."""

# contributed by Bastian Kleineidam

__revision__ = "$Id$"

import os
from distutils.core import Command
from stat import ST_MODE

class install_scripts(Command):

    description = "install scripts"

    user_options = [
        ('install-dir=', 'd', "directory to install to"),
        ('build-dir=','b', "build directory (where to install from)"),
        ('skip-build', None, "skip the build steps"),
    ]

    def initialize_options (self):
        self.install_dir = None
        self.build_dir = None
        self.skip_build = None

    def finalize_options (self):
        self.set_undefined_options('build', ('build_scripts', 'build_dir'))
        self.set_undefined_options ('install',
                                    ('install_scripts', 'install_dir'),
                                    ('skip_build', 'skip_build'),
                                   )

    def run (self):
        if not self.skip_build:
            self.run_peer('build_scripts')
        self.outfiles = self.copy_tree (self.build_dir, self.install_dir)
        if os.name == 'posix':
            # Set the executable bits (owner, group, and world) on
            # all the scripts we just installed.
            for file in self.get_outputs():
                if self.dry_run:
                    self.announce("changing mode of %s" % file)
                else:
                    mode = (os.stat(file)[ST_MODE]) | 0111
                    self.announce("changing mode of %s to %o" % (file, mode))
                    os.chmod(file, mode)

    def get_inputs (self):
        return self.distribution.scripts or []

    def get_outputs(self):
        return self.outfiles or []

# class install_scripts
