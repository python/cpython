# created 1999/03/13, Greg Ward

__revision__ = "$Id$"

import sys, os, string
from distutils.core import Command
from distutils.dir_util import copy_tree
from distutils.util import byte_compile

class install_lib (Command):

    description = "install all Python modules (extensions and pure Python)"

    user_options = [
        ('install-dir=', 'd', "directory to install to"),
        ('build-dir=','b', "build directory (where to install from)"),
        ('force', 'f', "force installation (overwrite existing files)"),
        ('compile', 'c', "compile .py to .pyc"),
        ('optimize', 'o', "compile .py to .pyo (optimized)"),
        ('skip-build', None, "skip the build steps"),
        ]
               
    boolean_options = ['force', 'compile', 'optimize', 'skip-build']


    def initialize_options (self):
        # let the 'install' command dictate our installation directory
        self.install_dir = None
        self.build_dir = None
        self.force = 0
        self.compile = 1
        self.optimize = 1
        self.skip_build = None

    def finalize_options (self):

        # Get all the information we need to install pure Python modules
        # from the umbrella 'install' command -- build (source) directory,
        # install (target) directory, and whether to compile .py files.
        self.set_undefined_options('install',
                                   ('build_lib', 'build_dir'),
                                   ('install_lib', 'install_dir'),
                                   ('force', 'force'),
                                   ('compile_py', 'compile'),
                                   ('optimize_py', 'optimize'),
                                   ('skip_build', 'skip_build'),
                                  )

    def run (self):

        # Make sure we have built everything we need first
        self.build()
        
        # Install everything: simply dump the entire contents of the build
        # directory to the installation directory (that's the beauty of
        # having a build directory!)
        outfiles = self.install()

        # (Optionally) compile .py to .pyc
        if outfiles is not None and self.distribution.has_pure_modules():
            self.bytecompile(outfiles)

    # run ()


    # -- Top-level worker functions ------------------------------------
    # (called from 'run()')

    def build (self):
        if not self.skip_build:
            if self.distribution.has_pure_modules():
                self.run_command('build_py')
            if self.distribution.has_ext_modules():
                self.run_command('build_ext')
        
    def install (self):
        if os.path.isdir(self.build_dir):
            outfiles = self.copy_tree(self.build_dir, self.install_dir)
        else:
            self.warn("'%s' does not exist -- no Python modules to install" %
                      self.build_dir)
            return
        return outfiles

    def bytecompile (self, files):
        byte_compile(files,
                     force=self.force,
                     verbose=self.verbose, dry_run=self.dry_run)


    # -- Utility methods -----------------------------------------------

    def _mutate_outputs (self, has_any, build_cmd, cmd_option, output_dir):

        if not has_any:
            return []

        build_cmd = self.get_finalized_command(build_cmd)
        build_files = build_cmd.get_outputs()
        build_dir = getattr(build_cmd, cmd_option)

        prefix_len = len(build_dir) + len(os.sep)
        outputs = []
        for file in build_files:
            outputs.append(os.path.join(output_dir, file[prefix_len:]))

        return outputs

    # _mutate_outputs ()

    def _bytecode_filenames (self, py_filenames):
        bytecode_files = []
        for py_file in py_filenames:
            bytecode = py_file + (__debug__ and "c" or "o")
            bytecode_files.append(bytecode)

        return bytecode_files
        

    # -- External interface --------------------------------------------
    # (called by outsiders)

    def get_outputs (self):
        """Return the list of files that would be installed if this command
        were actually run.  Not affected by the "dry-run" flag or whether
        modules have actually been built yet.
        """
        pure_outputs = \
            self._mutate_outputs(self.distribution.has_pure_modules(),
                                 'build_py', 'build_lib',
                                 self.install_dir)
        if self.compile:
            bytecode_outputs = self._bytecode_filenames(pure_outputs)
        else:
            bytecode_outputs = []

        ext_outputs = \
            self._mutate_outputs(self.distribution.has_ext_modules(),
                                 'build_ext', 'build_lib',
                                 self.install_dir)

        return pure_outputs + bytecode_outputs + ext_outputs

    # get_outputs ()

    def get_inputs (self):
        """Get the list of files that are input to this command, ie. the
        files that get installed as they are named in the build tree.
        The files in this list correspond one-to-one to the output
        filenames returned by 'get_outputs()'.
        """
        inputs = []
        
        if self.distribution.has_pure_modules():
            build_py = self.get_finalized_command('build_py')
            inputs.extend(build_py.get_outputs())

        if self.distribution.has_ext_modules():
            build_ext = self.get_finalized_command('build_ext')
            inputs.extend(build_ext.get_outputs())

        return inputs

# class install_lib
