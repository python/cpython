"""distutils.command.install

Implements the Distutils 'install' command."""

# created 1999/03/13, Greg Ward

__rcsid__ = "$Id$"

import sys, os, string
from distutils import sysconfig
from distutils.core import Command


class Install (Command):

    options = [('prefix=', None, "installation prefix"),
               ('execprefix=', None,
                "prefix for platform-specific files"),

               # Build directories: where to install from
               ('build-base=', None,
                "base build directory"),
               ('build-lib=', None,
                "build directory for non-platform-specific library files"),
               ('build-platlib=', None,
                "build directory for platform-specific library files"),

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

               # Where to install documentation (eventually!)
               ('doc-format=', None, "format of documentation to generate"),
               ('install-man=', None, "directory for Unix man pages"),
               ('install-html=', None, "directory for HTML documentation"),
               ('install-info=', None, "directory for GNU info files"),
               
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

        self.install_man = None
        self.install_html = None
        self.install_info = None


    def set_final_options (self):

        # Figure out the build directories, ie. where to install from
        self.set_peer_option ('build', 'basedir', self.build_base)
        self.set_undefined_options ('build',
                                    ('basedir', 'build_base'),
                                    ('libdir', 'build_lib'),
                                    ('platdir', 'build_platlib'))

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
            self.prefix = sys.prefix
        if self.exec_prefix is None:
            self.exec_prefix = sys.exec_prefix

        if self.install_lib is None:
            self.install_lib = \
                self.replace_sys_prefix ('LIBDEST', ('lib','python1.5'))
        if self.install_platlib is None:
            # XXX this should probably be DESTSHARED -- but why is there no
            # equivalent to DESTSHARED for the "site-packages" dir"?
            self.install_platlib = \
                self.replace_sys_prefix ('BINLIBDEST', ('lib','python1.5'), 1)

        if self.install_site_lib is None:
            self.install_site_lib = \
                os.path.join (self.install_lib, 'site-packages')
        if self.install_site_platlib is None:
            # XXX ugh! this puts platform-specific files in with shared files,
            # with no nice way to override it! (this might be a Python
            # problem, though, not a Distutils problem...)
            self.install_site_platlib = \
                os.path.join (self.install_lib, 'site-packages')

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
            sys_prefix = sys.exec_prefix
            my_prefix = self.exec_prefix
        else:
            sys_prefix = sys.prefix
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

            return apply (os.join, (my_prefix,) + fallback_postfix)

    # replace_sys_prefix ()
    

    def run (self):

        self.set_final_options ()

        # Install modules in two steps: "platform-shared" files (ie. pure
        # python modules) and platform-specific files (compiled C
        # extensions).

        self.run_peer ('install_py')

        # don't have an 'install_ext' command just yet!
        #self.run_peer ('install_ext'))

    # run ()

# class Install
