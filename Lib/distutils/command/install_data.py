"""distutils.command.install_data

Implements the Distutils 'install_data' command, for installing
platform-independent data files."""

# contributed by Bastian Kleineidam

__revision__ = "$Id$"

import os
from types import StringType
from distutils.core import Command
from distutils.util import change_root

class install_data (Command):

    description = "install data files"

    user_options = [
        ('install-dir=', 'd',
         "directory to install the files to"),
        ('root=', None,
         "install everything relative to this alternate root directory"),
        ]

    def initialize_options (self):
        self.install_dir = None
        self.outfiles = None
        self.root = None
        self.data_files = self.distribution.data_files

    def finalize_options (self):
        self.set_undefined_options('install',
	                           ('install_data', 'install_dir'),
				   ('root', 'root'),
				  )

    def run (self):
        self.mkpath(self.install_dir)
        for f in self.data_files:
            if type(f) == StringType:
                # its a simple file, so copy it
                self.copy_file(f, self.install_dir)
            else:
                # its a tuple with path to install to and a list of files
                dir = f[0]
                if not os.path.isabs(dir):
                    dir = os.path.join(self.install_dir, dir)
                elif self.root:
                    dir = change_root(self.root, dir)
                self.mkpath(dir)
                for data in f[1]:
                    self.copy_file(data, dir)

    def get_inputs (self):
        return self.data_files or []

    def get_outputs (self):
        return self.outfiles or []
