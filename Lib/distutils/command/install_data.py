"""distutils.command.install_data

Implements the Distutils 'install_data' command, for installing
platform-independent data files."""

# contributed by Bastian Kleineidam

__revision__ = "$Id$"

from distutils.cmd import install_misc

class install_data (install_misc):

    description = "install data files"

    def finalize_options (self):
        self._install_dir_from('install_data')

    def run (self):
        self._copy_files(self.distribution.data)
