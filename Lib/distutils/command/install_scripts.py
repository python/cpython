import os
from distutils.cmd import install_misc
from stat import ST_MODE

class install_scripts(install_misc):

    description = "install scripts"

    def finalize_options (self):
        self._install_dir_from('install_scripts')

    def run (self):
        self._copy_files(self.distribution.scripts)
        if os.name == 'posix':
            # Set the executable bits (owner, group, and world) on
            # all the scripts we just installed.
            files = self.get_outputs()
            for file in files:
                if self.dry_run:
                    self.announce("changing mode of %s" % file)
                else:
                    mode = (os.stat(file)[ST_MODE]) | 0111
                    self.announce("changing mode of %s to %o" % (file, mode))
                    os.chmod(file, mode)

# class install_scripts
