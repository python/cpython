"""Install scripts."""

# Contributed by Bastian Kleineidam

import os
from packaging.command.cmd import Command
from packaging import logger

class install_scripts(Command):

    description = "install scripts (Python or otherwise)"

    user_options = [
        ('install-dir=', 'd', "directory to install scripts to"),
        ('build-dir=','b', "build directory (where to install from)"),
        ('force', 'f', "force installation (overwrite existing files)"),
        ('skip-build', None, "skip the build steps"),
    ]

    boolean_options = ['force', 'skip-build']


    def initialize_options(self):
        self.install_dir = None
        self.force = False
        self.build_dir = None
        self.skip_build = None

    def finalize_options(self):
        self.set_undefined_options('build', ('build_scripts', 'build_dir'))
        self.set_undefined_options('install_dist',
                                   ('install_scripts', 'install_dir'),
                                   'force', 'skip_build')

    def run(self):
        if not self.skip_build:
            self.run_command('build_scripts')

        if not os.path.exists(self.build_dir):
            self.outfiles = []
            return

        self.outfiles = self.copy_tree(self.build_dir, self.install_dir)
        if os.name == 'posix':
            # Set the executable bits (owner, group, and world) on
            # all the scripts we just installed.
            for file in self.get_outputs():
                if self.dry_run:
                    logger.info("changing mode of %s", file)
                else:
                    mode = (os.stat(file).st_mode | 0o555) & 0o7777
                    logger.info("changing mode of %s to %o", file, mode)
                    os.chmod(file, mode)

    def get_inputs(self):
        return self.distribution.scripts or []

    def get_outputs(self):
        return self.outfiles or []
