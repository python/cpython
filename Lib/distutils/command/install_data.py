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
         "base directory for installing data files "
         "(default: installation base dir)"),
        ('root=', None,
         "install everything relative to this alternate root directory"),
        ('force', 'f', "force installation (overwrite existing files)"),
        ]

    boolean_options = ['force']

    def initialize_options (self):
        self.install_dir = None
        self.outfiles = []
        self.root = None
        self.force = 0

        self.data_files = self.distribution.data_files
        self.warn_dir = 1

    def finalize_options (self):
        self.set_undefined_options('install',
                                   ('install_data', 'install_dir'),
                                   ('root', 'root'),
                                   ('force', 'force'),
                                  )

    def run (self):
        self.mkpath(self.install_dir)
        for f in self.data_files:
            if type(f) == StringType:
                # it's a simple file, so copy it
                if self.warn_dir:
                    self.warn("setup script did not provide a directory for "
                              "'%s' -- installing right in '%s'" %
                              (f, self.install_dir))
                (out, _) = self.copy_file(f, self.install_dir)
                self.outfiles.append(out)
            else:
                # it's a tuple with path to install to and a list of files
                dir = f[0]
                if not os.path.isabs(dir):
                    dir = os.path.join(self.install_dir, dir)
                elif self.root:
                    dir = change_root(self.root, dir)
                self.mkpath(dir)
                for data in f[1]:
                    (out, _) = self.copy_file(data, dir)
                    self.outfiles.append(out)

    def get_inputs (self):
        return self.data_files or []

    def get_outputs (self):
        return self.outfiles
