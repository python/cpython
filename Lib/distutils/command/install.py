"""distutils.command.install

Implements the Distutils 'install' command."""

# created 1999/03/13, Greg Ward

__revision__ = "$Id$"

import sys, os, string
from types import *
from distutils.core import Command
from distutils.util import write_file
from distutils.errors import DistutilsOptionError

class install (Command):

    description = "install everything from build directory"

    user_options = [
        ('prefix=', None, "installation prefix"),
        ('exec-prefix=', None,
         "prefix for platform-specific files"),

        # Installation directories: where to put modules and packages
        ('install-lib=', None,
         "base Python library directory"),
        ('install-platlib=', None,
         "platform-specific Python library directory"),
        ('install-path=', None,
         "extra intervening directories to put below install-lib"),

        # Build directories: where to find the files to install
        ('build-base=', None,
         "base build directory"),
        ('build-lib=', None,
         "build directory for pure Python modules"),
        ('build-platlib=', None,
         "build directory for extension modules"),

        # Where to install documentation (eventually!)
        #('doc-format=', None, "format of documentation to generate"),
        #('install-man=', None, "directory for Unix man pages"),
        #('install-html=', None, "directory for HTML documentation"),
        #('install-info=', None, "directory for GNU info files"),

        # Flags for 'build_py'
        #('compile-py', None, "compile .py to .pyc"),
        #('optimize-py', None, "compile .py to .pyo (optimized)"),
        ]


    def initialize_options (self):

        # Don't define 'prefix' or 'exec_prefix' so we can know when the
        # command is run whether the user supplied values
        self.prefix = None
        self.exec_prefix = None

        # The actual installation directories are determined only at
        # run-time, so the user can supply just prefix (and exec_prefix?)
        # as a base for everything else
        self.install_lib = None
        self.install_platlib = None
        self.install_path = None

        self.build_base = None
        self.build_lib = None
        self.build_platlib = None

        self.install_man = None
        self.install_html = None
        self.install_info = None

        self.compile_py = 1
        self.optimize_py = 1


    def finalize_options (self):

        # XXX this method is where the default installation directories
        # for modules and extension modules are determined.  (Someday,
        # the default installation directories for scripts,
        # documentation, and whatever else the Distutils can build and
        # install will also be determined here.)  Thus, this is a pretty
        # important place to fiddle with for anyone interested in
        # installation schemes for the Python library.  Issues that
        # are not yet resolved to my satisfaction:
        #   * how much platform dependence should be here, and
        #     how much can be pushed off to sysconfig (or, better, the
        #     Makefiles parsed by sysconfig)?
        #   * how independent of Python version should this be -- ie.
        #     should we have special cases to handle Python 1.5 and
        #     older, and then do it "the right way" for 1.6?  Or should
        #     we install a site.py along with Distutils under pre-1.6
        #     Python to take care of the current deficiencies in
        #     Python's library installation scheme?
        #
        # Currently, this method has hacks to distinguish POSIX from
        # non-POSIX systems (for installation of site-local modules),
        # and assumes the Python 1.5 installation tree with no site.py
        # to fix things.

        # Figure out actual installation directories; the basic principle
        # is: ...

        sys_prefix = os.path.normpath (sys.prefix)
        sys_exec_prefix = os.path.normpath (sys.exec_prefix)

        if self.prefix is None:
            if self.exec_prefix is not None:
                raise DistutilsOptionError, \
                      "you may not supply exec_prefix without prefix"
            self.prefix = sys_prefix
        else:
            # This is handy to guarantee that self.prefix is normalized --
            # but it could be construed as rude to go normalizing a
            # user-supplied path (they might like to see their "../" or
            # symlinks in the installation feedback).
            self.prefix = os.path.normpath (self.prefix)
            
        if self.exec_prefix is None:
            if self.prefix == sys_prefix:
                self.exec_prefix = sys_exec_prefix
            else:
                self.exec_prefix = self.prefix
        else:
            # Same as above about handy versus rude to normalize user's
            # exec_prefix.
            self.exec_prefix = os.path.normpath (self.exec_prefix)

        if self.distribution.ext_modules: # any extensions to install?
            effective_prefix = self.exec_prefix
        else:
            effective_prefix = self.prefix

        if os.name == 'posix':
            if self.install_lib is None:
                if self.prefix == sys_prefix:
                    self.install_lib = \
                        os.path.join (effective_prefix,
                                      "lib",
                                      "python" + sys.version[:3],
                                      "site-packages")
                else:
                    self.install_lib = \
                        os.path.join (effective_prefix,
                                      "lib",
                                      "python")      # + sys.version[:3] ???
            # end if self.install_lib ...

            if self.install_platlib is None:
                if self.exec_prefix == sys_exec_prefix:
                    self.install_platlib = \
                        os.path.join (effective_prefix,
                                      "lib",
                                      "python" + sys.version[:3],
                                      "site-packages")
                else:
                    self.install_platlib = \
                        os.path.join (effective_prefix,
                                      "lib",
                                      "python")      # + sys.version[:3] ???
            # end if self.install_platlib ...

        else:
            raise DistutilsPlatformError, \
                  "duh, I'm clueless (for now) about installing on %s" % os.name

        # end if/else on os.name
                    

        # 'path_file' and 'extra_dirs' are how we handle distributions that
        # want to be installed to their own directory, but aren't
        # package-ized yet.  'extra_dirs' is just a directory under
        # 'install_lib' or 'install_platlib' where top-level modules will
        # actually be installed; 'path_file' is the basename of a .pth file
        # to drop in 'install_lib' or 'install_platlib' (depending on the
        # distribution).  Very often they will be the same, which is why we
        # allow them to be supplied as a string or 1-tuple as well as a
        # 2-element comma-separated string or a 2-tuple.

        # XXX this will drop a .pth file in install_{lib,platlib} even if
        # they're not one of the site-packages directories: this is wrong!
        # we need to suppress path_file in those cases, and warn if
        # "install_lib/extra_dirs" is not in sys.path.

        if self.install_path is None:
            self.install_path = self.distribution.install_path

        if self.install_path is not None:
            if type (self.install_path) is StringType:
                self.install_path = string.split (self.install_path, ',')

            if len (self.install_path) == 1:
                path_file = extra_dirs = self.install_path[0]
            elif len (self.install_path) == 2:
                (path_file, extra_dirs) = self.install_path
            else:
                raise DistutilsOptionError, \
                      "'install_path' option must be a list, tuple, or " + \
                      "comma-separated string with 1 or 2 elements"

            # install path has slashes in it -- might need to convert to
            # local form
            if string.find (extra_dirs, '/') and os.name != "posix":
                extra_dirs = string.split (extra_dirs, '/')
                extra_dirs = apply (os.path.join, extra_dirs)
        else:
            path_file = None
            extra_dirs = ''

        # XXX should we warn if path_file and not extra_dirs (in which case
        # the path file would be harmless but pointless)
        self.path_file = path_file
        self.extra_dirs = extra_dirs


        # Figure out the build directories, ie. where to install from
        self.set_peer_option ('build', 'build_base', self.build_base)
        self.set_undefined_options ('build',
                                    ('build_base', 'build_base'),
                                    ('build_lib', 'build_lib'),
                                    ('build_platlib', 'build_platlib'))

        # Punt on doc directories for now -- after all, we're punting on
        # documentation completely!

    # finalize_options ()


    def run (self):

        # Obviously have to build before we can install
        self.run_peer ('build')

        # Install modules in two steps: "platform-shared" files (ie. pure
        # Python modules) and platform-specific files (compiled C
        # extensions).  Note that 'install_py' is smart enough to install
        # pure Python modules in the "platlib" directory if we built any
        # extensions.
        if self.distribution.packages or self.distribution.py_modules:
            self.run_peer ('install_py')
        if self.distribution.ext_modules:
            self.run_peer ('install_ext')

        if self.path_file:
            self.create_path_file ()

    # run ()


    def create_path_file (self):

        if self.distribution.ext_modules:
            base = self.platbase
        else:
            base = self.base

        filename = os.path.join (base, self.path_file + ".pth")
        self.execute (write_file,
                      (filename, [self.extra_dirs]),
                      "creating %s" % filename)


# class Install
