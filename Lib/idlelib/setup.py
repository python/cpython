import os, glob, sys
from distutils.core import setup
from distutils.command.build_py import build_py
from distutils.command.install_lib import install_lib
import idlever

idle_name = "idle"

# Name of 'package' to be installed in site-packages:
pkgname = idle_name + "lib"

try:
    pos = sys.argv.index("--check-tkinter")
except ValueError:
    pass
else:
    del sys.argv[pos]
    try:
        import _tkinter
    except ImportError:
        print >>sys.stderr, "Cannot install IDLE without _tkinter"
        raise SystemExit

try:
    pkg_dir = os.path.join(os.environ['SRCDIR'], 'Tools', idle_name)
except KeyError:
    pkg_dir = "."

# the normal build_py would not incorporate anything but .py files
txt_files = ['extend.txt', 'help.txt', 'CREDITS.txt', 'HISTORY.txt',
             'INSTALL.txt', 'LICENSE.txt', 'NEWS.txt', 'README.txt']
txt_files += ['config-extensions.def', 'config-highlight.def',
              'config-keys.def', 'config-main.def']
txt_files += [idle_name + '.bat', idle_name + '.pyw']

Icons = glob.glob1(os.path.join(pkg_dir, "Icons"), "*.gif")

class IDLE_Builder(build_py):
    def get_plain_outfile(self, build_dir, package, file):
        # like get_module_outfile, but does not append .py
        outfile_path = [build_dir] + list(package) + [file]
        return apply(os.path.join, outfile_path)

    def run(self):
        # Copies all .py files, then also copies the txt and gif files
        build_py.run(self)
        for name in txt_files:
            outfile = self.get_plain_outfile(self.build_lib, [pkgname], name)
            dir = os.path.dirname(outfile)
            self.mkpath(dir)
            self.copy_file(os.path.join(pkg_dir, name), outfile,
                           preserve_mode = 0)
        for name in Icons:
            outfile = self.get_plain_outfile(self.build_lib,
                                             [pkgname, "Icons"], name)
            dir = os.path.dirname(outfile)
            self.mkpath(dir)
            self.copy_file(os.path.join(pkg_dir, "Icons", name),
                           outfile, preserve_mode = 0)

    def get_source_files(self):
        # returns the .py files, the .txt and .def files, and the icons
        icons = [os.path.join(pkg_dir, "Icons",name) for name in Icons]
        txts = [os.path.join(pkg_dir, name) for name in txt_files]
        return build_py.get_source_files(self) + txt_files + icons

    def get_outputs(self, include_bytecode=1):
        # returns the built files
        outputs = build_py.get_outputs(self, include_bytecode)
        if not include_bytecode:
            return outputs
        for name in txt_files:
            filename = self.get_plain_outfile(self.build_lib, [pkgname], name)
            outputs.append(filename)
        for name in Icons:
            filename = self.get_plain_outfile(self.build_lib,
                                              [pkgname, "Icons"], name)
            outputs.append(filename)
        return outputs

# Arghhh. install_lib thinks that all files returned from build_py's
# get_outputs are bytecode files

class IDLE_Installer(install_lib):
    def _bytecode_filenames(self, files):
        files = [n for n in files if n.endswith('.py')]
        return install_lib._bytecode_filenames(self, files)

setup(name="IDLEfork",
      version = idlever.IDLE_VERSION,
      description = "IDLEfork, the Developmental Python IDE",
      author = "Guido van Rossum et. al.",
      author_email = "idle-dev@python.org",
      license = "PSF: www.python.org",
      url = "https://sourceforge.net/projects/idlefork/",
      long_description =
"""IDLE is a Tkinter based IDE for Python. It is written in 100% pure
Python and works both on Windows and Unix. It features a multi-window
text editor with multiple undo, Python colorizing, and many other
things, as well as a Python shell window and a debugger.

IDLEfork is a separate line of development which was initiated by
David Scherer at CMU as part of VPython.  It features execution in a
separate process which is newly initiated for each run.  At version 0.9
the RPC was changed to incorporate code by GvR, which supports the
debugger.  IDLEfork also incorporates a GUI configuration utilility.
For further details, refer to idlefork.sourceforge.net.
""",

      cmdclass = {'build_py':IDLE_Builder,
                  'install_lib':IDLE_Installer},
      package_dir = {pkgname: pkg_dir},
      packages = [pkgname],
      scripts = [os.path.join(pkg_dir, idle_name)]
      )
