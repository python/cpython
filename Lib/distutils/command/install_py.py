# created 1999/03/13, Greg Ward

__rcsid__ = "$Id$"

import sys, string
from distutils.core import Command
from distutils.util import copy_tree

class InstallPy (Command):

    options = [('dir=', 'd', "directory to install to"),
               ('build-dir=' 'b', "build directory (where to install from)"),
               ('compile', 'c', "compile .py to .pyc"),
               ('optimize', 'o', "compile .py to .pyo (optimized)"),
              ]
               

    def set_default_options (self):
        # let the 'install' command dictate our installation directory
        self.dir = None
        self.build_dir = None
        self.compile = 1
        self.optimize = 1

    def set_final_options (self):
        # If we don't have a 'dir' value, we'll have to ask the 'install'
        # command for one.  (This usually means the user ran 'install_py'
        # directly, rather than going through 'install' -- so in reality,
        # 'find_command_obj()' will create an 'install' command object,
        # which we then query.

        self.set_undefined_options ('install',
                                    ('build_lib', 'build_dir'),
                                    ('install_site_lib', 'dir'),
                                    ('compile_py', 'compile'),
                                    ('optimize_py', 'optimize'))


    def run (self):

        self.set_final_options ()

        # Dump entire contents of the build directory to the installation
        # directory (that's the beauty of having a build directory!)
        outfiles = self.copy_tree (self.build_dir, self.dir)
                   
        # (Optionally) compile .py to .pyc
        # XXX hey! we can't control whether we optimize or not; that's up
        # to the invocation of the current Python interpreter (at least
        # according to the py_compile docs).  That sucks.

        if self.compile:
            from py_compile import compile

            for f in outfiles:
                # XXX can't assume this filename mapping!
                out_fn = string.replace (f, '.py', '.pyc')
                
                self.make_file (f, out_fn, compile, (f,),
                                "compiling %s -> %s" % (f, out_fn),
                                "compilation of %s skipped" % f)

        # XXX ignore self.optimize for now, since we don't really know if
        # we're compiling optimally or not, and couldn't pick what to do
        # even if we did know.  ;-(

    # run ()

# class InstallPy
