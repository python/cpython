"""distutils.command.config

Implements the Distutils 'config' command, a (mostly) empty command class
that exists mainly to be sub-classed by specific module distributions and
applications.  The idea is that while every "config" command is different,
at least they're all named the same, and users always see "config" in the
list of standard commands.  Also, this is a good place to put common
configure-like tasks: "try to compile this C code", or "figure out where
this header file lives".
"""

# created 2000/05/29, Greg Ward

__revision__ = "$Id$"

import os, string
from distutils.core import Command
from distutils.errors import DistutilsExecError


LANG_EXT = {'c': '.c',
            'c++': '.cxx'}

class config (Command):

    description = "prepare to build"

    user_options = [
        ('compiler=', None,
         "specify the compiler type"),
        ('cc=', None,
         "specify the compiler executable"),
        ('include-dirs=', 'I',
         "list of directories to search for header files"),
        ('define=', 'D',
         "C preprocessor macros to define"),
        ('undef=', 'U',
         "C preprocessor macros to undefine"),
        ('libraries=', 'l',
         "external C libraries to link with"),
        ('library-dirs=', 'L',
         "directories to search for external C libraries"),
        ]


    # The three standard command methods: since the "config" command
    # does nothing by default, these are empty.

    def initialize_options (self):
        self.compiler = None
        self.cc = None
        self.include_dirs = None
        #self.define = None
        #self.undef = None
        self.libraries = None
        self.library_dirs = None

    def finalize_options (self):
        pass

    def run (self):
        pass


    # Utility methods for actual "config" commands.  The interfaces are
    # loosely based on Autoconf macros of similar names.  Sub-classes
    # may use these freely.

    def _check_compiler (self):
        """Check that 'self.compiler' really is a CCompiler object;
        if not, make it one.
        """
        # We do this late, and only on-demand, because this is an expensive
        # import.
        from distutils.ccompiler import CCompiler, new_compiler
        if not isinstance(self.compiler, CCompiler):
            self.compiler = new_compiler (compiler=self.compiler,
                                          verbose=self.verbose, # for now
                                          dry_run=self.dry_run,
                                          force=1)
            if self.include_dirs:
                self.compiler.set_include_dirs(self.include_dirs)
            if self.libraries:
                self.compiler.set_libraries(self.libraries)
            if self.library_dirs:
                self.compiler.set_library_dirs(self.library_dirs)


    def _gen_temp_sourcefile (self, body, lang):
        filename = "_configtest" + LANG_EXT[lang]
        file = open(filename, "w")
        file.write(body)
        file.close()
        return filename

    def _compile (self, body, lang):
        src = self._gen_temp_sourcefile(body, lang)
        (obj,) = self.compiler.compile([src])
        return (src, obj)

    def _link (self, body, lang):
        (src, obj) = self._compile(body, lang)
        exe = os.path.splitext(os.path.basename(src))[0]
        self.compiler.link_executable([obj], exe)
        return (src, obj, exe)

    def _clean (self, *filenames):
        self.announce("removing: " + string.join(filenames))
        for filename in filenames:
            try:
                os.remove(filename)
            except OSError:
                pass


    # XXX no 'try_cpp()' or 'search_cpp()' since the CCompiler interface
    # does not provide access to the preprocessor.  This is an oversight
    # that should be fixed.

    # XXX these ignore the dry-run flag: what to do, what to do? even if
    # you want a dry-run build, you still need some sort of configuration
    # info.  My inclination is to make it up to the real config command to
    # consult 'dry_run', and assume a default (minimal) configuration if
    # true.  The problem with trying to do it here is that you'd have to
    # return either true or false from all the 'try' methods, neither of
    # which is correct.

    def try_compile (self, body, lang="c"):
        """Try to compile a source file that consists of the text in 'body'
        (a multi-line string).  Return true on success, false
        otherwise.
        """
        from distutils.ccompiler import CompileError
        self._check_compiler()
        try:
            (src, obj) = self._compile(body, lang)
            ok = 1
        except CompileError:
            ok = 0

        self.announce(ok and "success!" or "failure.")
        self._clean(src, obj)
        return ok

    def try_link (self, body, lang="c"):
        """Try to compile and link a source file (to an executable) that
        consists of the text in 'body' (a multi-line string).  Return true
        on success, false otherwise.
        """
        from distutils.ccompiler import CompileError, LinkError
        self._check_compiler()
        try:
            (src, obj, exe) = self._link(body, lang)
            ok = 1
        except (CompileError, LinkError):
            ok = 0

        self.announce(ok and "success!" or "failure.")
        self._clean(src, obj, exe)
        return ok
            
    def try_run (self, body, lang="c"):
        """Try to compile, link to an executable, and run a program that
        consists of the text in 'body'.  Return true on success, false
        otherwise.
        """
        from distutils.ccompiler import CompileError, LinkError
        self._check_compiler()
        try:
            (src, obj, exe) = self._link(body, lang)
            self.spawn([exe])
            ok = 1
        except (CompileError, LinkError, DistutilsExecError):
            ok = 0

        self.announce(ok and "success!" or "failure.")
        self._clean(src, obj, exe)
        return ok

# class config
