import os,glob
from distutils.core import setup
from distutils.command.build_py import build_py
from distutils.command.install_lib import install_lib
import idlever

# name of idle package
idlelib = "idlelib"

# the normal build_py would not incorporate the .txt files
txt_files = ['config-unix.txt','config-win.txt','config.txt']
Icons = glob.glob1("Icons","*.gif")
class idle_build_py(build_py):
    def get_plain_outfile(self, build_dir, package, file):
        # like get_module_outfile, but does not append .py
        outfile_path = [build_dir] + list(package) + [file]
        return apply(os.path.join, outfile_path)

    def run(self):
        # Copies all .py files, then also copies the txt and gif files
        build_py.run(self)
        assert self.packages == [idlelib]
        for name in txt_files:
            outfile = self.get_plain_outfile(self.build_lib, [idlelib], name)
            dir = os.path.dirname(outfile)
            self.mkpath(dir)
            self.copy_file(name, outfile, preserve_mode = 0)
        for name in Icons:
            outfile = self.get_plain_outfile(self.build_lib,
                                             [idlelib,"Icons"], name)
            dir = os.path.dirname(outfile)
            self.mkpath(dir)
            self.copy_file(os.path.join("Icons",name),
                           outfile, preserve_mode = 0)

    def get_source_files(self):
        # returns the .py files, the .txt files, and the icons
        icons = [os.path.join("Icons",name) for name in Icons]
        return build_py.get_source_files(self)+txt_files+icons

    def get_outputs(self, include_bytecode=1):
        # returns the built files
        outputs = build_py.get_outputs(self, include_bytecode)
        if not include_bytecode:
            return outputs
        for name in txt_files:
            filename = self.get_plain_outfile(self.build_lib,
                                              [idlelib], name)
            outputs.append(filename)
        for name in Icons:
            filename = self.get_plain_outfile(self.build_lib,
                                              [idlelib,"Icons"], name)
            outputs.append(filename)
        return outputs

# Arghhh. install_lib thinks that all files returned from build_py's
# get_outputs are bytecode files
class idle_install_lib(install_lib):
    def _bytecode_filenames(self, files):
        files = [n for n in files if n.endswith('.py')]
        return install_lib._bytecode_filenames(self,files)


setup(name="IDLE",
      version = idlever.IDLE_VERSION,
      description = "IDLE, the Python IDE",
      author = "Guido van Rossum",
      author_email = "guido@python.org",
      #url =
      long_description =
"""IDLE is a Tkinter based IDE for Python. It is written in 100% pure
Python and works both on Windows and Unix. It features a multi-window
text editor with multiple undo, Python colorizing, and many other things,
as well as a Python shell window and a debugger.""",

      cmdclass = {'build_py':idle_build_py,
                  'install_lib':idle_install_lib},
      package_dir = {idlelib:'.'},
      packages = [idlelib],
      scripts = ['idle']
      )
