"""distutils.command.install

Implements the Distutils 'install' command."""

# created 1999/03/13, Greg Ward

__rcsid__ = "$Id$"

import sys, os, string
from types import *
from distutils import sysconfig
from distutils.core import Command
from distutils.util import write_file


class Install (Command):

    options = [('prefix=', None, "installation prefix"),
               ('exec-prefix=', None,
                "prefix for platform-specific files"),

               # Build directories: where to install from
               ('build-base=', None,
                "base build directory"),
               ('build-lib=', None,
                "build directory for pure Python modules"),
               ('build-platlib=', None,
                "build directory for extension modules"),

               # Installation directories: where to put modules and packages
               ('install-lib=', None,
                "base Python library directory"),
               ('install-platlib=', None,
                "platform-specific Python library directory"),
               ('install-site-lib=', None,
                "directory for site-specific packages and modules"),
               ('install-site-platlib=', None,
                "platform-specific site directory"),
               ('install-scheme=', None,
                "install to 'system' or 'site' library directory?"),
               ('install-path=', None,
                "extra intervening directories to put below install-lib"),

               # Where to install documentation (eventually!)
               ('doc-format=', None, "format of documentation to generate"),
               ('install-man=', None, "directory for Unix man pages"),
               ('install-html=', None, "directory for HTML documentation"),
               ('install-info=', None, "directory for GNU info files"),
               
               # Flags for 'build_py'
               ('compile-py', None, "compile .py to .pyc"),
               ('optimize-py', None, "compile .py to .pyo (optimized)"),
              ]


    def set_default_options (self):

        self.build_base = None
        self.build_lib = None
        self.build_platlib = None

        # Don't define 'prefix' or 'exec_prefix' so we can know when the
        # command is run whether the user supplied values
        self.prefix = None
        self.exec_prefix = None

        # These two, we can supply real values for! (because they're
        # not directories, and don't have a confusing multitude of
        # possible derivations)
        #self.install_scheme = 'site'
        self.doc_format = None

        # The actual installation directories are determined only at
        # run-time, so the user can supply just prefix (and exec_prefix?)
        # as a base for everything else
        self.install_lib = None
        self.install_platlib = None
        self.install_site_lib = None
        self.install_site_platlib = None
        self.install_path = None

        self.install_man = None
        self.install_html = None
        self.install_info = None

        self.compile_py = 1
        self.optimize_py = 1


    def set_final_options (self):

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

        # Figure out the build directories, ie. where to install from
        self.set_peer_option ('build', 'build_base', self.build_base)
        self.set_undefined_options ('build',
                                    ('build_base', 'build_base'),
                                    ('build_lib', 'build_lib'),
                                    ('build_platlib', 'build_platlib'))

        # Figure out actual installation directories; the basic principle
        # is: if the user supplied nothing, then use the directories that
        # Python was built and installed with (ie. the compiled-in prefix
        # and exec_prefix, and the actual installation directories gleaned
        # by sysconfig).  If the user supplied a prefix (and possibly
        # exec_prefix), then we generate our own installation directories,
        # following any pattern gleaned from sysconfig's findings.  If no
        # such pattern can be gleaned, then we'll just make do and try to
        # ape the behaviour of Python's configure script.

        if self.prefix is None:         # user didn't override
            self.prefix = os.path.normpath (sys.prefix)
        if self.exec_prefix is None:
            self.exec_prefix = os.path.normpath (sys.exec_prefix)

        if self.install_lib is None:
            self.install_lib = \
                self.replace_sys_prefix ('LIBDEST', ('lib','python1.5'))
        if self.install_platlib is None:
            # XXX this should probably be DESTSHARED -- but why is there no
            # equivalent to DESTSHARED for the "site-packages" dir"?
            self.install_platlib = \
                self.replace_sys_prefix ('BINLIBDEST', ('lib','python1.5'), 1)


        # Here is where we decide where to install most library files: on
        # POSIX systems, they go to 'site-packages' under the install_lib
        # (determined above -- typically /usr/local/lib/python1.x).  Note
        # that on POSIX systems, platform-specific files belong in
        # 'site-packages' under install_platlib.  (The actual rule is that
        # a module distribution that includes *any* platform-specific files
        # -- ie. extension modules -- goes under install_platlib.  This
        # solves the "can't find extension module in a package" problem.)
        # On non-POSIX systems, install_lib and install_platlib are the
        # same (eg. "C:\Program Files\Python\Lib" on Windows), as are
        # install_site_lib and install_site_platlib (eg.
        # "C:\Program Files\Python" on Windows) -- everything will be dumped
        # right into one of the install_site directories.  (It doesn't
        # really matter *which* one, of course, but I'll observe decorum
        # and do it properly.)

        # 'base' and 'platbase' are the base directories for installing
        # site-local files, eg. "/usr/local/lib/python1.5/site-packages"
        # or "C:\Program Files\Python"
        if os.name == 'posix':
            self.base = os.path.join (self.install_lib,
                                      'site-packages')
            self.platbase = os.path.join (self.install_platlib,
                                          'site-packages')
        else:
            self.base = self.prefix
            self.platbase = self.exec_prefix
        
        # 'path_file' and 'extra_dirs' are how we handle distributions
        # that need to be installed to their own directory, but aren't
        # package-ized yet.  'extra_dirs' is just a directory under
        # 'base' or 'platbase' where toplevel modules will actually be
        # installed; 'path_file' is the basename of a .pth file to drop
        # in 'base' or 'platbase' (depending on the distribution).  Very
        # often they will be the same, which is why we allow them to be
        # supplied as a string or 1-tuple as well as a 2-element
        # comma-separated string or a 2-tuple.
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


        if self.install_site_lib is None:
            self.install_site_lib = os.path.join (self.base,
                                                  extra_dirs)

        if self.install_site_platlib is None:
            self.install_site_platlib = os.path.join (self.platbase,
                                                      extra_dirs)

        #if self.install_scheme == 'site':
        #    install_lib = self.install_site_lib
        #    install_platlib = self.install_site_platlib
        #elif self.install_scheme == 'system':
        #    install_lib = self.install_lib
        #    install_platlib = self.install_platlib
        #else:
        #    # XXX new exception for this kind of misbehaviour?
        #    raise DistutilsArgError, \
        #          "invalid install scheme '%s'" % self.install_scheme
            

        # Punt on doc directories for now -- after all, we're punting on
        # documentation completely!

    # set_final_options ()


    def replace_sys_prefix (self, config_attr, fallback_postfix, use_exec=0):
        """Attempts to glean a simple pattern from an installation
           directory available as a 'sysconfig' attribute: if the
           directory name starts with the "system prefix" (the one
           hard-coded in the Makefile and compiled into Python),
           then replace it with the current installation prefix and
           return the "relocated" installation directory."""

        if use_exec:
            sys_prefix = os.path.normpath (sys.exec_prefix)
            my_prefix = self.exec_prefix
        else:
            sys_prefix = os.path.normpath (sys.prefix)
            my_prefix = self.prefix

        val = getattr (sysconfig, config_attr)
        if string.find (val, sys_prefix) == 0:
            # If the sysconfig directory starts with the system prefix,
            # then we can "relocate" it to the user-supplied prefix --
            # assuming, of course, it is different from the system prefix.

            if sys_prefix == my_prefix:
                return val
            else:
                return my_prefix + val[len(sys_prefix):]

        else:
            # Otherwise, just tack the "fallback postfix" onto the
            # user-specified prefix.

            return apply (os.path.join, (my_prefix,) + fallback_postfix)

    # replace_sys_prefix ()
    

    def run (self):

        # Obviously have to build before we can install
        self.run_peer ('build')

        # Install modules in two steps: "platform-shared" files (ie. pure
        # python modules) and platform-specific files (compiled C
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
