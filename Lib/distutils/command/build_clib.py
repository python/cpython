"""distutils.command.build_lib

Implements the Distutils 'build_lib' command, to build a C/C++ library
that is included in the module distribution and needed by an extension
module."""

# created (an empty husk) 1999/12/18, Greg Ward
# fleshed out 2000/02/03-04

__rcsid__ = "$Id$"


# XXX this module has *lots* of code ripped-off quite transparently from
# build_ext.py -- not surprisingly really, as the work required to build
# a static library from a collection of C source files is not really all
# that different from what's required to build a shared object file from
# a collection of C source files.  Nevertheless, I haven't done the
# necessary refactoring to account for the overlap in code between the
# two modules, mainly because a number of subtle details changed in the
# cut 'n paste.  Sigh.

import os, string
from types import *
from distutils.core import Command
from distutils.errors import *
from distutils.ccompiler import new_compiler


class build_lib (Command):

    user_options = [
        ('debug', 'g',
         "compile with debugging information"),
        ]

    def initialize_options (self):
        # List of libraries to build
        self.libraries = None

        # Compilation options for all libraries
        self.include_dirs = None
        self.define = None
        self.undef = None
        self.debug = None

    # initialize_options()

    def finalize_options (self):
        self.set_undefined_options ('build',
                                    ('debug', 'debug'))
        self.libraries = self.distribution.libraries
        if self.include_dirs is None:
            self.include_dirs = self.distribution.include_dirs or []
        if type (self.include_dirs) is StringType:
            self.include_dirs = string.split (self.include_dirs,
                                              os.pathsep)

        # XXX same as for build_ext -- what about 'self.define' and
        # 'self.undef' ?

    # finalize_options()


    def run (self):

        if not self.libraries:
            return
        self.check_library_list (self.libraries)

        # Yech -- this is cut 'n pasted from build_ext.py!
        self.compiler = new_compiler (plat=os.environ.get ('PLAT'),
                                      verbose=self.verbose,
                                      dry_run=self.dry_run,
                                      force=self.force)
        if self.include_dirs is not None:
            self.compiler.set_include_dirs (self.include_dirs)
        if self.define is not None:
            # 'define' option is a list of (name,value) tuples
            for (name,value) in self.define:
                self.compiler.define_macro (name, value)
        if self.undef is not None:
            for macro in self.undef:
                self.compiler.undefine_macro (macro)

        self.build_libraries (self.libraries)

    # run()


    def check_library_list (self, libraries):
        """Ensure that the list of libraries (presumably provided as a
           command option 'libraries') is valid, i.e. it is a list of
           2-tuples, where the tuples are (library_name, build_info_dict).
           Raise DistutilsValueError if the structure is invalid anywhere;
           just returns otherwise."""

        # Yechh, blecch, ackk: this is ripped straight out of build_ext.py,
        # with only names changed to protect the innocent!

        if type (libraries) is not ListType:
            raise DistutilsValueError, \
                  "'libraries' option must be a list of tuples"

        for lib in libraries:
            if type (lib) is not TupleType and len (lib) != 2:
                raise DistutilsValueError, \
                      "each element of 'libraries' must a 2-tuple"

            if type (lib[0]) is not StringType:
                raise DistutilsValueError, \
                      "first element of each tuple in 'libraries' " + \
                      "must be a string (the library name)"
            if type (lib[1]) is not DictionaryType:
                raise DistutilsValueError, \
                      "second element of each tuple in 'libraries' " + \
                      "must be a dictionary (build info)"
        # for lib

    # check_library_list ()


    def build_libraries (self, libraries):

        compiler = self.compiler

        for (lib_name, build_info) in libraries:
            sources = build_info.get ('sources')
            if sources is None or type (sources) not in (ListType, TupleType):
                raise DistutilsValueError, \
                      ("in 'libraries' option (library '%s'), " +
                       "'sources' must be present and must be " +
                       "a list of source filenames") % lib_name
            sources = list (sources)

            self.announce ("building '%s' library" % lib_name)

            # Extract the directory the library is intended to go in --
            # note translation from "universal" slash-separated form to
            # current platform's pathname convention (so we can use the
            # string for actual filesystem use).
            path = tuple (string.split (lib_name, '/')[:-1])
            if path:
                lib_dir = apply (os.path.join, path)
            else:
                lib_dir = ''

            # First, compile the source code to object files in the library
            # directory.  (This should probably change to putting object
            # files in a temporary build directory.)
            macros = build_info.get ('macros')
            include_dirs = build_info.get ('include_dirs')
            objects = self.compiler.compile (sources,
                                             macros=macros,
                                             include_dirs=include_dirs,
                                             output_dir=lib_dir,
                                             debug=self.debug)

            # Now "link" the object files together into a static library.
            # (On Unix at least, this isn't really linking -- it just
            # builds an archive.  Whatever.)
            self.compiler.link_static_lib (objects, lib_name, debug=self.debug)

        # for libraries

    # build_libraries ()
                

# class BuildLib
