# created 1999/03/13, Greg Ward

__rcsid__ = "$Id$"

import sys, string
from distutils.core import Command
from distutils.util import copy_tree

class InstallPy (Command):

    options = [('install-dir=', 'd', "directory to install to"),
               ('build-dir=','b', "build directory (where to install from)"),
               ('compile', 'c', "compile .py to .pyc"),
               ('optimize', 'o', "compile .py to .pyo (optimized)"),
              ]
               

    def set_default_options (self):
        # let the 'install' command dictate our installation directory
        self.install_dir = None
        self.build_dir = None
        self.compile = 1
        self.optimize = 1

    def set_final_options (self):

        # Find out from the 'build_ext' command if we were asked to build
        # any extensions.  If so, that means even pure-Python modules in
        # this distribution have to be installed to the "platlib"
        # directory.
        extensions = self.get_peer_option ('build_ext', 'extensions')
        if extensions:
            dir_option = 'install_site_platlib'
        else:
            dir_option = 'install_site_lib'

        # Get all the information we need to install pure Python modules
        # from the umbrella 'install' command -- build (source) directory,
        # install (target) directory, and whether to compile .py files.
        self.set_undefined_options ('install',
                                    ('build_lib', 'build_dir'),
                                    (dir_option, 'install_dir'),
                                    ('compile_py', 'compile'),
                                    ('optimize_py', 'optimize'))


    def run (self):

        # Dump entire contents of the build directory to the installation
        # directory (that's the beauty of having a build directory!)
        outfiles = self.copy_tree (self.build_dir, self.install_dir)
                   
        # (Optionally) compile .py to .pyc
        # XXX hey! we can't control whether we optimize or not; that's up
        # to the invocation of the current Python interpreter (at least
        # according to the py_compile docs).  That sucks.

        if self.compile:
            from py_compile import compile

            for f in outfiles:
                # XXX can't assume this filename mapping!

                # only compile the file if it is actually a .py file
                if f[-3:] == '.py':
                    out_fn = string.replace (f, '.py', '.pyc')
                    
                    self.make_file (f, out_fn, compile, (f,),
                                    "compiling %s -> %s" % (f, out_fn),
                                    "compilation of %s skipped" % f)
                    
        # XXX ignore self.optimize for now, since we don't really know if
        # we're compiling optimally or not, and couldn't pick what to do
        # even if we did know.  ;-(

    # run ()

# class InstallPy
