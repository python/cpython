"""distutils.command.install

Implements the Distutils 'install' command."""

# created 1999/03/13, Greg Ward

__revision__ = "$Id$"

import sys, os, string
from types import *
from distutils.core import Command
from distutils import sysconfig
from distutils.util import write_file, native_path, subst_vars, change_root
from distutils.errors import DistutilsOptionError
from glob import glob

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
        ('root=', None,
         "install everything relative to this alternate root directory"),

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

        # For lazy debuggers who just want to test the install
        # commands without rerunning "build" all the time
        ('skip-build', None,
         "skip rebuilding everything (for testing/debugging)"),

        # Where to install documentation (eventually!)
        #('doc-format=', None, "format of documentation to generate"),
        #('install-man=', None, "directory for Unix man pages"),
        #('install-html=', None, "directory for HTML documentation"),
        #('install-info=', None, "directory for GNU info files"),

        ('record=', None,
         "filename in which to record list of installed files"),
        ]

    # 'sub_commands': a list of commands this command might have to run to
    # get its work done.  Each command is represented as a tuple (method,
    # command) where 'method' is the name of a method to call that returns
    # true if 'command' (the sub-command name, a string) needs to be run.
    # If 'method' is None, assume that 'command' must always be run.
    sub_commands = [('has_lib', 'install_lib'),
                    ('has_scripts', 'install_scripts'),
                    ('has_data', 'install_data'),
                   ]


    def initialize_options (self):

        # High-level options: these select both an installation base
        # and scheme.
        self.prefix = None
        self.exec_prefix = None
        self.home = None

        # These select only the installation base; it's up to the user to
        # specify the installation scheme (currently, that means supplying
        # the --install-{platlib,purelib,scripts,data} options).
        self.install_base = None
        self.install_platbase = None
        self.root = None

        # These options are the actual installation directories; if not
        # supplied by the user, they are filled in using the installation
        # scheme implied by prefix/exec-prefix/home and the contents of
        # that installation scheme.
        self.install_purelib = None     # for pure module distributions
        self.install_platlib = None     # non-pure (dists w/ extensions)
        self.install_lib = None         # set to either purelib or platlib
        self.install_scripts = None
        self.install_data = None

        # These two are for putting non-packagized distributions into their
        # own directory and creating a .pth file if it makes sense.
        # 'extra_path' comes from the setup file; 'install_path_file' is
        # set only if we determine that it makes sense to install a path
        # file.
        self.extra_path = None
        self.install_path_file = 0

        self.skip_build = 0

        # These are only here as a conduit from the 'build' command to the
        # 'install_*' commands that do the real work.  ('build_base' isn't
        # actually used anywhere, but it might be useful in future.)  They
        # are not user options, because if the user told the install
        # command where the build directory is, that wouldn't affect the
        # build command.
        self.build_base = None
        self.build_lib = None

        # Not defined yet because we don't know anything about
        # documentation yet.
        #self.install_man = None
        #self.install_html = None
        #self.install_info = None

        self.record = None

    def finalize_options (self):

        # This method (and its pliant slaves, like 'finalize_unix()',
        # 'finalize_other()', and 'select_scheme()') is where the default
        # installation directories for modules, extension modules, and
        # anything else we care to install from a Python module
        # distribution.  Thus, this code makes a pretty important policy
        # statement about how third-party stuff is added to a Python
        # installation!  Note that the actual work of installation is done
        # by the relatively simple 'install_*' commands; they just take
        # their orders from the installation directory options determined
        # here.

        # Check for errors/inconsistencies in the options; first, stuff
        # that's wrong on any platform.

        if ((self.prefix or self.exec_prefix or self.home) and
            (self.install_base or self.install_platbase)):
            raise DistutilsOptionError, \
                  ("must supply either prefix/exec-prefix/home or " +
                   "install-base/install-platbase -- not both")

        # Next, stuff that's wrong (or dubious) only on certain platforms.
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

        self.dump_dirs ("pre-finalize_xxx")

        if os.name == 'posix':
            self.finalize_unix ()
        else:
            self.finalize_other ()

        self.dump_dirs ("post-finalize_xxx()")

        # Expand configuration variables, tilde, etc. in self.install_base
        # and self.install_platbase -- that way, we can use $base or
        # $platbase in the other installation directories and not worry
        # about needing recursive variable expansion (shudder).

        self.config_vars = {'py_version_short': sys.version[0:3],
                            'sys_prefix': sysconfig.PREFIX,
                            'sys_exec_prefix': sysconfig.EXEC_PREFIX,
                           }
        self.expand_basedirs ()

        self.dump_dirs ("post-expand_basedirs()")

        # Now define config vars for the base directories so we can expand
        # everything else.
        self.config_vars['base'] = self.install_base
        self.config_vars['platbase'] = self.install_platbase

        from pprint import pprint
        print "config vars:"
        pprint (self.config_vars)

        # Expand "~" and configuration variables in the installation
        # directories.
        self.expand_dirs ()

        self.dump_dirs ("post-expand_dirs()")

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

        # If a new root directory was supplied, make all the installation
        # dirs relative to it.
        if self.root is not None:
            for name in ('lib', 'purelib', 'platlib', 'scripts', 'data'):
                attr = "install_" + name
                new_val = change_root (self.root, getattr (self, attr))
                setattr (self, attr, new_val)

        self.dump_dirs ("after prepending root")

        # Find out the build directories, ie. where to install from.
        self.set_undefined_options ('build',
                                    ('build_base', 'build_base'),
                                    ('build_lib', 'build_lib'))

        # Punt on doc directories for now -- after all, we're punting on
        # documentation completely!

    # finalize_options ()


    # hack for debugging output
    def dump_dirs (self, msg):
        from distutils.fancy_getopt import longopt_xlate
        print msg + ":"
        for opt in self.user_options:
            opt_name = opt[0]
            if opt_name[-1] == "=":
                opt_name = opt_name[0:-1]
            opt_name = string.translate (opt_name, longopt_xlate)
            val = getattr (self, opt_name)
            print "  %s: %s" % (opt_name, val)


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
        for key in ('purelib', 'platlib', 'scripts', 'data'):
            attrname = 'install_' + key
            if getattr(self, attrname) is None:
                setattr(self, attrname, scheme[key])


    def _expand_attrs (self, attrs):
        for attr in attrs:
            val = getattr (self, attr)
            if val is not None:
                if os.name == 'posix':
                    val = os.path.expanduser (val)
                val = subst_vars (val, self.config_vars)
                setattr (self, attr, val)


    def expand_basedirs (self):
        self._expand_attrs (['install_base',
                             'install_platbase',
                             'root'])        

    def expand_dirs (self):
        self._expand_attrs (['install_purelib',
                             'install_platlib',
                             'install_lib',
                             'install_scripts',
                             'install_data',])


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


    def get_sub_commands (self):
        """Return the list of subcommands that we need to run.  This is
        based on the 'subcommands' class attribute: each tuple in that list
        can name a method that we call to determine if the subcommand needs
        to be run for the current distribution."""
        commands = []
        for (method, cmd_name) in self.sub_commands:
            if method is not None:
                method = getattr(self, method)
            if method is None or method():
                commands.append(cmd_name)
        return commands


    def run (self):

        # Obviously have to build before we can install
        if not self.skip_build:
            self.run_peer ('build')

        # Run all sub-commands (at least those that need to be run)
        for cmd_name in self.get_sub_commands():
            self.run_peer (cmd_name)

        if self.path_file:
            self.create_path_file ()

        # write list of installed files, if requested.
        if self.record:
            outputs = self.get_outputs()
            if self.root:               # strip any package prefix
                root_len = len(self.root)
                for counter in xrange (len (outputs)):
                    outputs[counter] = outputs[counter][root_len:]
            self.execute(write_file,
                         (self.record, outputs),
                         "writing list of installed files to '%s'" %
                         self.record)

        normalized_path = map (os.path.normpath, sys.path)
        if (not (self.path_file and self.install_path_file) and
            os.path.normpath (self.install_lib) not in normalized_path):
            self.warn (("modules installed to '%s', which is not in " +
                        "Python's module search path (sys.path) -- " +
                        "you'll have to change the search path yourself") %
                       self.install_lib)

    # run ()


    def has_lib (self):
        """Return true if the current distribution has any Python
        modules to install."""
        return (self.distribution.has_pure_modules() or
                self.distribution.has_ext_modules())

    def has_scripts (self):
        return self.distribution.has_scripts()

    def has_data (self):
        return self.distribution.has_data_files()


    def get_outputs (self):
        # This command doesn't have any outputs of its own, so just
        # get the outputs of all its sub-commands.
        outputs = []
        for cmd_name in self.get_sub_commands():
            cmd = self.find_peer (cmd_name)
            outputs.extend (cmd.get_outputs())

        return outputs


    def get_inputs (self):
        # XXX gee, this looks familiar ;-(
        inputs = []
        for cmd_name in self.get_sub_commands():
            cmd = self.find_peer (cmd_name)
            inputs.extend (cmd.get_inputs())

        return inputs


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

# class install
