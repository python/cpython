"""distutils.extension

Provides the Extension class, used to describe C/C++ extension
modules in setup scripts."""

# created 2000/05/30, Greg Ward

__revision__ = "$Id$"

from types import *


# This class is really only used by the "build_ext" command, so it might
# make sense to put it in distutils.command.build_ext.  However, that
# module is already big enough, and I want to make this class a bit more
# complex to simplify some common cases ("foo" module in "foo.c") and do
# better error-checking ("foo.c" actually exists).
# 
# Also, putting this in build_ext.py means every setup script would have to
# import that large-ish module (indirectly, through distutils.core) in
# order to do anything.

class Extension:
    """Just a collection of attributes that describes an extension
    module and everything needed to build it (hopefully in a portable
    way, but there are hooks that let you can be as unportable as you
    need).

    Instance attributes:
      name : string
        the full name of the extension, including any packages -- ie.
        *not* a filename or pathname, but Python dotted name
      sources : [string]
        list of source filenames, relative to the distribution root
        (where the setup script lives), in Unix form (slash-separated)
        for portability.  Source files may be C, C++, SWIG (.i),
        platform-specific resource files, or whatever else is recognized
        by the "build_ext" command as source for a Python extension.
      include_dirs : [string]
        list of directories to search for C/C++ header files (in Unix
        form for portability)
      define_macros : [(name : string, value : string|None)]
        list of macros to define; each macro is defined using a 2-tuple,
        where 'value' is either the string to define it to or None to
        define it without a particular value (equivalent of "#define
        FOO" in source or -DFOO on Unix C compiler command line)
      undef_macros : [string]
        list of macros to undefine explicitly
      library_dirs : [string]
        list of directories to search for C/C++ libraries at link time
      libraries : [string]
        list of library names (not filenames or paths) to link against
      runtime_library_dirs : [string]
        list of directories to search for C/C++ libraries at run time
        (for shared extensions, this is when the extension is loaded)
      extra_objects : [string]
        list of extra files to link with (eg. object files not implied
        by 'sources', static library that must be explicitly specified,
        binary resource files, etc.)
      extra_compile_args : [string]
        any extra platform- and compiler-specific information to use
        when compiling the source files in 'sources'.  For platforms and
        compilers where "command line" makes sense, this is typically a
        list of command-line arguments, but for other platforms it could
        be anything.
      extra_link_args : [string]
        any extra platform- and compiler-specific information to use
        when linking object files together to create the extension (or
        to create a new static Python interpreter).  Similar
        interpretation as for 'extra_compile_args'.
      export_symbols : [string]
        list of symbols to be exported from a shared extension.  Not
        used on all platforms, and not generally necessary for Python
        extensions, which typically export exactly one symbol: "init" +
        extension_name.
    """

    def __init__ (self, name, sources,
                  include_dirs=None,
                  define_macros=None,
                  undef_macros=None,
                  library_dirs=None,
                  libraries=None,
                  runtime_library_dirs=None,
                  extra_objects=None,
                  extra_compile_args=None,
                  extra_link_args=None,
                  export_symbols=None,
                 ):

        assert type(name) is StringType, "'name' must be a string"
        assert (type(sources) is ListType and
                len(sources) >= 1 and
                map(type, sources) == [StringType]*len(sources)), \
                "'sources' must be a non-empty list of strings"

        self.name = name
        self.sources = sources
        self.include_dirs = include_dirs or []
        self.define_macros = define_macros or []
        self.undef_macros = undef_macros or []
        self.library_dirs = library_dirs or []
        self.libraries = libraries or []
        self.runtime_library_dirs = runtime_library_dirs or []
        self.extra_objects = extra_objects or []
        self.extra_compile_args = extra_compile_args or []
        self.extra_link_args = extra_link_args or []
        self.export_symbols = export_symbols

# class Extension
