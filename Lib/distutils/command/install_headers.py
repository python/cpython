"""distutils.command.install_headers

Implements the Distutils 'install_headers' command, to install C/C++ header
files to the Python include directory."""

# created 2000/05/26, Greg Ward

__revision__ = "$Id$"

from distutils.core import Command


class install_headers (Command):

    description = "install C/C++ header files"

    user_options = [('install-dir=', 'd',
                     "directory to install header files to"),
                   ]


    def initialize_options (self):
        self.install_dir = None

    def finalize_options (self):
        self.set_undefined_options('install',
                                   ('install_headers', 'install_dir'))

    def run (self):
        headers = self.distribution.headers
        if not headers:
            return

        self.mkpath(self.install_dir)
        for header in headers:
            self.copy_file(header, self.install_dir)

    # run()

# class install_headers
