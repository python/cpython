"""distutils.command.install

Implements the Distutils 'install' command."""

# created 1999/03/13, Greg Ward

__revision__ = "$Id$"

import sys, os, string
from types import *
from distutils.core import Command
from distutils.util import write_file, native_path, subst_vars
from distutils.errors import DistutilsOptionError

INSTALL_SCHEMES = {
    'unix_prefix': {
        'purelib': '$base/lib/python$py_version_short/site-packages',
        'platlib': '$platbase/lib/python$py_version_short/site-packages',
        'scripts': '$base/bin',
        'data'   : '$base/share',
        },
    'unix_home': {
        'purelib': '$base/lib/python',
        'platlib': '$base/lib/python',
        'scripts': '$base/bin',
        'data'   : '$base/share',
        },
    'nt': {
        'purelib': '$base',
        'platlib': '$base',
        'scripts': '$base\\Scripts',
        'data'   : '$base\\Data',
        },
    'mac': {
        'purelib': '$base:Lib',
        'platlib': '$base:Mac:PlugIns',
        'scripts': '$base:Scripts',
        'data'   : '$base:Data',
        }
    }


class install (Command):

    description = "install everything from build directory"

    user_options = [
        # Select installation scheme and set base director(y|ies)
        ('prefix=', None,
         "installation prefix"),
        ('exec-prefix=', None,
         "(Unix only) prefix for platform-specific files"),
        ('home=', None,
         "(Unix only) home directory to install under"),

        # Or, just set the base director(y|ies)
        ('install-base=', None,
         "base installation directory (instead of --prefix or --home)"),
        ('install-platbase=', None,
         "base installation directory for platform-specific files " +
         "(instead of --exec-prefix or --home)"),

        # Or, explicitly set the installation scheme
        ('install-purelib=', None,
         "installation directory for pure Python module distributions"),
        ('install-platlib=', None,
         "installation directory for non-pure module distributions"),
        ('install-lib=', None,
         "installation directory for all module distributions " +
         "(overrides --install-purelib and --install-platlib)"),

        ('install-scripts=', None,
         "installation directory for Python scripts"),
        ('install-data=', None,
         "installation directory for data files"),

        # Build directories: where to find the files to install
        ('build-base=', None,
         "base build directory"),
        ('build-lib=', None,
         "build directory for all Python modules"),

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
        self.home = None

        self.install_base = None
        self.install_platbase = None

        # The actual installation directories are determined only at
        # run-time, so the user can supply just prefix (and exec_prefix?)
        # as a base for everything else
        self.install_purelib = None
        self.install_platlib = None
        self.install_lib = None

        self.install_scripts = None
        self.install_data = None

        self.build_base = None
        self.build_lib = None
        self.build_platlib = None

        self.extra_path = None
        self.install_path_file = 0

        #self.install_man = None
        #self.install_html = None
        #self.install_info = None

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



        # Logic:
        #   * any: (prefix or exec-prefix or home) and (base or platbase)
        #     supplied: error
        #   * Windows/Mac OS: exec-prefix or home supplied: warn and ignore
        # 
        #   * Unix: home set
        #     (select the unix_home scheme)
        #   * Unix: neither prefix nor home set
        #     (set prefix=sys_prefix and carry on)
        #   * Unix: prefix set but not exec-prefix
        #     (set exec-prefix=prefix and carry on)
        #   * Unix: prefix set
        #     (select the unix_prefix scheme)
        # 
        #   * Windows/Mac OS: prefix not set
        #     (set prefix = sys_prefix and carry on)
        #   * Windows/Mac OS: prefix set
        #     (select the appropriate scheme)

        # "select a scheme" means:
        #   - set install-base and install-platbase
        #   - subst. base/platbase/version into the values of the
        #     particular scheme dictionary
        #   - use the resultings strings to set install-lib, etc.

        sys_prefix = os.path.normpath (sys.prefix)
        sys_exec_prefix = os.path.normpath (sys.exec_prefix)

        # Check for errors/inconsistencies in the options
        if ((self.prefix or self.exec_prefix or self.home) and
            (self.install_base or self.install_platbase)):
            raise DistutilsOptionError, \
                  ("must supply either prefix/exec-prefix/home or " +
                   "install-base/install-platbase -- not both")

        if os.name == 'posix':
            if self.home and (self.prefix or self.exec_prefix):
                raise DistutilsOptionError, \
                      ("must supply either home or prefix/exec-prefix -- " +
                       "not both")
        else:
            if self.exec_prefix:
                self.warn ("exec-prefix option ignored on this platform")
                self.exec_prefix = None
            if self.home:
                self.warn ("home option ignored on this platform")
                self.home = None

        # Now the interesting logic -- so interesting that we farm it out
        # to other methods.  The goal of these methods is to set the final
        # values for the install_{lib,scripts,data,...}  options, using as
        # input a heady brew of prefix, exec_prefix, home, install_base,
        # install_platbase, user-supplied versions of
        # install_{purelib,platlib,lib,scripts,data,...}, and the
        # INSTALL_SCHEME dictionary above.  Phew!

        if os.name == 'posix':
            self.finalize_unix ()
        else:
            self.finalize_other ()

        # Expand "~" and configuration variables in the installation
        # directories.
        self.expand_dirs ()

        # Pick the actual directory to install all modules to: either
        # install_purelib or install_platlib, depending on whether this
        # module distribution is pure or not.  Of course, if the user
        # already specified install_lib, use their selection.
        if self.install_lib is None:
            if self.distribution.ext_modules: # has extensions: non-pure
                self.install_lib = self.install_platlib
            else:
                self.install_lib = self.install_purelib
                    
        # Well, we're not actually fully completely finalized yet: we still
        # have to deal with 'extra_path', which is the hack for allowing
        # non-packagized module distributions (hello, Numerical Python!) to
        # get their own directories.
        self.handle_extra_path ()
        self.install_libbase = self.install_lib # needed for .pth file
        self.install_lib = os.path.join (self.install_lib, self.extra_dirs)

        # Figure out the build directories, ie. where to install from
        self.set_peer_option ('build', 'build_base', self.build_base)
        self.set_undefined_options ('build',
                                    ('build_base', 'build_base'),
                                    ('build_lib', 'build_lib'),
                                    ('build_platlib', 'build_platlib'))

        # Punt on doc directories for now -- after all, we're punting on
        # documentation completely!

    # finalize_options ()


    def finalize_unix (self):
        
        if self.install_base is not None or self.install_platbase is not None:
            if ((self.install_lib is None and
                 self.install_purelib is None and
                 self.install_platlib is None) or
                self.install_scripts is None or
                self.install_data is None):
                raise DistutilsOptionError, \
                      "install-base or install-platbase supplied, but " + \
                      "installation scheme is incomplete"

            return

        if self.home is not None:
            self.install_base = self.install_platbase = self.home
            self.select_scheme ("unix_home")
        else:
            if self.prefix is None:
                if self.exec_prefix is not None:
                    raise DistutilsOptionError, \
                          "must not supply exec-prefix without prefix"

                self.prefix = os.path.normpath (sys.prefix)
                self.exec_prefix = os.path.normpath (sys.exec_prefix)
                self.install_path_file = 1

            else:
                if self.exec_prefix is None:
                    self.exec_prefix = self.prefix


            # XXX since we don't *know* that a user-supplied prefix really
            # points to another Python installation, we can't be sure that
            # writing a .pth file there will actually work -- so we don't
            # try.  That is, we only set 'install_path_file' if the user
            # didn't supply prefix.  There are certainly circumstances
            # under which we *should* install a .pth file when the user
            # supplies a prefix, namely when that prefix actually points to
            # another Python installation.  Hmmm.

            self.install_base = self.prefix
            self.install_platbase = self.exec_prefix
            self.select_scheme ("unix_prefix")

    # finalize_unix ()


    def finalize_other (self):          # Windows and Mac OS for now

        if self.prefix is None:
            self.prefix = os.path.normpath (sys.prefix)
            self.install_path_file = 1

        # XXX same caveat regarding 'install_path_file' as in
        # 'finalize_unix()'.

        self.install_base = self.install_platbase = self.prefix
        try:
            self.select_scheme (os.name)
        except KeyError:
            raise DistutilsPlatformError, \
                  "I don't know how to install stuff on '%s'" % os.name

    # finalize_other ()


    def select_scheme (self, name):

        # it's the caller's problem if they supply a bad name!
        scheme = INSTALL_SCHEMES[name]

        vars = { 'base': self.install_base,
                 'platbase': self.install_platbase,
                 'py_version_short': sys.version[0:3],
               }

        for key in ('purelib', 'platlib', 'scripts', 'data'):
            val = subst_vars (scheme[key], vars)
            setattr (self, 'install_' + key, val)


    def expand_dirs (self):

        # XXX probably don't want to 'expanduser()' on Windows or Mac
        # XXX should use 'util.subst_vars()' with our own set of
        #     configuration variables

        for att in ('base', 'platbase',
                    'purelib', 'platlib', 'lib',
                    'scripts', 'data'):
            fullname = "install_" + att
            val = getattr (self, fullname)
            if val is not None:
                setattr (self, fullname,
                         os.path.expandvars (os.path.expanduser (val)))


    def handle_extra_path (self):

        if self.extra_path is None:
            self.extra_path = self.distribution.extra_path

        if self.extra_path is not None:
            if type (self.extra_path) is StringType:
                self.extra_path = string.split (self.extra_path, ',')

            if len (self.extra_path) == 1:
                path_file = extra_dirs = self.extra_path[0]
            elif len (self.extra_path) == 2:
                (path_file, extra_dirs) = self.extra_path
            else:
                raise DistutilsOptionError, \
                      "'extra_path' option must be a list, tuple, or " + \
                      "comma-separated string with 1 or 2 elements"

            # convert to local form in case Unix notation used (as it
            # should be in setup scripts)
            extra_dirs = native_path (extra_dirs)

        else:
            path_file = None
            extra_dirs = ''

        # XXX should we warn if path_file and not extra_dirs? (in which
        # case the path file would be harmless but pointless)
        self.path_file = path_file
        self.extra_dirs = extra_dirs

    # handle_extra_path ()


    def run (self):

        # Obviously have to build before we can install
        self.run_peer ('build')

        # Install modules in two steps: "platform-shared" files (ie. pure
        # Python modules) and platform-specific files (compiled C
        # extensions).  Note that 'install_py' is smart enough to install
        # pure Python modules in the "platlib" directory if we built any
        # extensions.

        # XXX this should become one command, 'install_lib', since
        # all modules are "built" into the same directory now

        if self.distribution.packages or self.distribution.py_modules:
            self.run_peer ('install_py')
        #if self.distribution.ext_modules:
        #    self.run_peer ('install_ext')

        if self.path_file:
            self.create_path_file ()

        normalized_path = map (os.path.normpath, sys.path)
        if (not (self.path_file and self.install_path_file) and
            os.path.normpath (self.install_lib) not in normalized_path):
            self.warn (("modules installed to '%s', which is not in " +
                        "Python's module search path (sys.path) -- " +
                        "you'll have to change the search path yourself") %
                       self.install_lib)

    # run ()


    def create_path_file (self):
        filename = os.path.join (self.install_libbase,
                                 self.path_file + ".pth")
        if self.install_path_file:
            self.execute (write_file,
                          (filename, [self.extra_dirs]),
                          "creating %s" % filename)
        else:
            self.warn (("path file '%s' not created for alternate or custom " +
                        "installation (path files only work with standard " +
                        "installations)") %
                       filename)

# class Install
