from distutils.cmd import install_misc

class install_scripts(install_misc):

    description = "install scripts"
    # XXX needed?
    user_options = [('install-dir=', 'd', "directory to install to")]

    def finalize_options (self):
        self._install_dir_from('install_scripts')

    def run (self):
        self._copydata(self.distribution.scripts)

    def get_outputs(self):
        return self._outputdata(self.distribution.scripts)
