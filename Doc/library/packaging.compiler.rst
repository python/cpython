:mod:`packaging.compiler` --- Compiler classes
==============================================

.. module:: packaging.compiler
   :synopsis: Compiler classes to build C/C++ extensions or libraries.


This subpackage contains an abstract base class representing a compiler and
concrete implementations for common compilers.  The compiler classes should not
be instantiated directly, but created using the :func:`new_compiler` factory
function.  Compiler types provided by Packaging are listed in
:ref:`packaging-standard-compilers`.


Public functions
----------------

.. function:: new_compiler(plat=None, compiler=None, dry_run=False, force=False)

   Factory function to generate an instance of some
   :class:`~.ccompiler.CCompiler` subclass for the requested platform or
   compiler type.

   If no argument is given for *plat* and *compiler*, the default compiler type
   for the platform (:attr:`os.name`) will be used: ``'unix'`` for Unix and
   Mac OS X, ``'msvc'`` for Windows.

   If *plat* is given, it must be one of ``'posix'``, ``'darwin'`` or ``'nt'``.
   An invalid value will not raise an exception but use the default compiler
   type for the current platform.

   .. XXX errors should never pass silently; this behavior is particularly
      harmful when a compiler type is given as first argument

   If *compiler* is given, *plat* will be ignored, allowing you to get for
   example a ``'unix'`` compiler object under Windows or an ``'msvc'`` compiler
   under Unix.  However, not all compiler types can be instantiated on every
   platform.


.. function:: customize_compiler(compiler)

   Do any platform-specific customization of a CCompiler instance.  Mainly
   needed on Unix to plug in the information that varies across Unices and is
   stored in CPython's Makefile.


.. function:: gen_lib_options(compiler, library_dirs, runtime_library_dirs, libraries)

   Generate linker options for searching library directories and linking with
   specific libraries.  *libraries* and *library_dirs* are, respectively, lists
   of library names (not filenames!) and search directories.  Returns a list of
   command-line options suitable for use with some compiler (depending on the
   two format strings passed in).


.. function:: gen_preprocess_options(macros, include_dirs)

   Generate C preprocessor options (:option:`-D`, :option:`-U`, :option:`-I`) as
   used by at least two types of compilers: the typical Unix compiler and Visual
   C++. *macros* is the usual thing, a list of 1- or 2-tuples, where ``(name,)``
   means undefine (:option:`-U`) macro *name*, and ``(name, value)`` means
   define (:option:`-D`) macro *name* to *value*.  *include_dirs* is just a list
   of directory names to be added to the header file search path (:option:`-I`).
   Returns a list of command-line options suitable for either Unix compilers or
   Visual C++.


.. function:: get_default_compiler(osname, platform)

   Determine the default compiler to use for the given platform.

   *osname* should be one of the standard Python OS names (i.e. the ones
   returned by ``os.name``) and *platform* the common value returned by
   ``sys.platform`` for the platform in question.

   The default values are ``os.name`` and ``sys.platform``.


.. function:: set_compiler(location)

   Add or change a compiler


.. function:: show_compilers()

   Print list of available compilers (used by the :option:`--help-compiler`
   options to :command:`build`, :command:`build_ext`, :command:`build_clib`).


.. _packaging-standard-compilers:

Standard compilers
------------------

Concrete subclasses of :class:`~.ccompiler.CCompiler` are provided in submodules
of the :mod:`packaging.compiler` package.  You do not need to import them, using
:func:`new_compiler` is the public API to use.  This table documents the
standard compilers; be aware that they can be replaced by other classes on your
platform.

=============== ======================================================== =======
name            description                                              notes
=============== ======================================================== =======
``'unix'``      typical Unix-style command-line C compiler               [#]_
``'msvc'``      Microsoft compiler                                       [#]_
``'bcpp'``      Borland C++ compiler
``'cygwin'``    Cygwin compiler (Windows port of GCC)
``'mingw32'``   Mingw32 port of GCC (same as Cygwin in no-Cygwin mode)
=============== ======================================================== =======


.. [#] The Unix compiler class assumes this behavior:

       * macros defined with :option:`-Dname[=value]`

       * macros undefined with :option:`-Uname`

       * include search directories specified with :option:`-Idir`

       * libraries specified with :option:`-llib`

       * library search directories specified with :option:`-Ldir`

       * compile handled by :program:`cc` (or similar) executable with
         :option:`-c` option: compiles :file:`.c` to :file:`.o`

       * link static library handled by :program:`ar` command (possibly with
         :program:`ranlib`)

       * link shared library handled by :program:`cc` :option:`-shared`


.. [#] On Windows, extension modules typically need to be compiled with the same
       compiler that was used to compile CPython (for example Microsoft Visual
       Studio .NET 2003 for CPython 2.4 and 2.5).  The AMD64 and Itanium
       binaries are created using the Platform SDK.

       Under the hood, there are actually two different subclasses of
       :class:`~.ccompiler.CCompiler` defined: one is compatible with MSVC 2005
       and 2008, the other works with older versions.  This should not be a
       concern for regular use of the functions in this module.

       Packaging will normally choose the right compiler, linker etc. on its
       own.  To override this choice, the environment variables
       *DISTUTILS_USE_SDK* and *MSSdk* must be both set.  *MSSdk* indicates that
       the current environment has been setup by the SDK's ``SetEnv.Cmd``
       script, or that the environment variables had been registered when the
       SDK was installed; *DISTUTILS_USE_SDK* indicates that the user has made
       an explicit choice to override the compiler selection done by Packaging.

       .. TODO document the envvars in Doc/using and the man page


:mod:`packaging.compiler.ccompiler` --- CCompiler base class
============================================================

.. module:: packaging.compiler.ccompiler
   :synopsis: Abstract CCompiler class.


This module provides the abstract base class for the :class:`CCompiler`
classes.  A :class:`CCompiler` instance can be used for all the compile and
link steps needed to build a single project. Methods are provided to set
options for the compiler --- macro definitions, include directories, link path,
libraries and the like.

.. class:: CCompiler(dry_run=False, force=False)

   The abstract base class :class:`CCompiler` defines the interface that must be
   implemented by real compiler classes.  The class also has some utility
   methods used by several compiler classes.

   The basic idea behind a compiler abstraction class is that each instance can
   be used for all the compile/link steps in building a single project.  Thus,
   attributes common to all of those compile and link steps --- include
   directories, macros to define, libraries to link against, etc. --- are
   attributes of the compiler instance.  To allow for variability in how
   individual files are treated, most of those attributes may be varied on a
   per-compilation or per-link basis.

   The constructor for each subclass creates an instance of the Compiler object.
   Flags are *dry_run* (don't actually execute
   the steps) and *force* (rebuild everything, regardless of dependencies).  All
   of these flags default to ``False`` (off). Note that you probably don't want to
   instantiate :class:`CCompiler` or one of its subclasses directly - use the
   :func:`new_compiler` factory function instead.

   The following methods allow you to manually alter compiler options for the
   instance of the Compiler class.


   .. method:: CCompiler.add_include_dir(dir)

      Add *dir* to the list of directories that will be searched for header
      files.  The compiler is instructed to search directories in the order in
      which they are supplied by successive calls to :meth:`add_include_dir`.


   .. method:: CCompiler.set_include_dirs(dirs)

      Set the list of directories that will be searched to *dirs* (a list of
      strings). Overrides any preceding calls to :meth:`add_include_dir`;
      subsequent calls to :meth:`add_include_dir` add to the list passed to
      :meth:`set_include_dirs`. This does not affect any list of standard
      include directories that the compiler may search by default.


   .. method:: CCompiler.add_library(libname)

      Add *libname* to the list of libraries that will be included in all links
      driven by this compiler object.  Note that *libname* should *not* be the
      name of a file containing a library, but the name of the library itself:
      the actual filename will be inferred by the linker, the compiler, or the
      compiler class (depending on the platform).

      The linker will be instructed to link against libraries in the order they
      were supplied to :meth:`add_library` and/or :meth:`set_libraries`.  It is
      perfectly valid to duplicate library names; the linker will be instructed
      to link against libraries as many times as they are mentioned.


   .. method:: CCompiler.set_libraries(libnames)

      Set the list of libraries to be included in all links driven by this
      compiler object to *libnames* (a list of strings).  This does not affect
      any standard system libraries that the linker may include by default.


   .. method:: CCompiler.add_library_dir(dir)

      Add *dir* to the list of directories that will be searched for libraries
      specified to :meth:`add_library` and :meth:`set_libraries`.  The linker
      will be instructed to search for libraries in the order they are supplied
      to :meth:`add_library_dir` and/or :meth:`set_library_dirs`.


   .. method:: CCompiler.set_library_dirs(dirs)

      Set the list of library search directories to *dirs* (a list of strings).
      This does not affect any standard library search path that the linker may
      search by default.


   .. method:: CCompiler.add_runtime_library_dir(dir)

      Add *dir* to the list of directories that will be searched for shared
      libraries at runtime.


   .. method:: CCompiler.set_runtime_library_dirs(dirs)

      Set the list of directories to search for shared libraries at runtime to
      *dirs* (a list of strings).  This does not affect any standard search path
      that the runtime linker may search by default.


   .. method:: CCompiler.define_macro(name, value=None)

      Define a preprocessor macro for all compilations driven by this compiler
      object. The optional parameter *value* should be a string; if it is not
      supplied, then the macro will be defined without an explicit value and the
      exact outcome depends on the compiler used (XXX true? does ANSI say
      anything about this?)


   .. method:: CCompiler.undefine_macro(name)

      Undefine a preprocessor macro for all compilations driven by this compiler
      object.  If the same macro is defined by :meth:`define_macro` and
      undefined by :meth:`undefine_macro` the last call takes precedence
      (including multiple redefinitions or undefinitions).  If the macro is
      redefined/undefined on a per-compilation basis (i.e. in the call to
      :meth:`compile`), then that takes precedence.


   .. method:: CCompiler.add_link_object(object)

      Add *object* to the list of object files (or analogues, such as explicitly
      named library files or the output of "resource compilers") to be included
      in every link driven by this compiler object.


   .. method:: CCompiler.set_link_objects(objects)

      Set the list of object files (or analogues) to be included in every link
      to *objects*.  This does not affect any standard object files that the
      linker may include by default (such as system libraries).

   The following methods implement methods for autodetection of compiler
   options, providing some functionality similar to GNU :program:`autoconf`.


   .. method:: CCompiler.detect_language(sources)

      Detect the language of a given file, or list of files. Uses the instance
      attributes :attr:`language_map` (a dictionary), and :attr:`language_order`
      (a list) to do the job.


   .. method:: CCompiler.find_library_file(dirs, lib, debug=0)

      Search the specified list of directories for a static or shared library file
      *lib* and return the full path to that file.  If *debug* is true, look for a
      debugging version (if that makes sense on the current platform).  Return
      ``None`` if *lib* wasn't found in any of the specified directories.


   .. method:: CCompiler.has_function(funcname, includes=None, include_dirs=None, libraries=None, library_dirs=None)

      Return a boolean indicating whether *funcname* is supported on the current
      platform.  The optional arguments can be used to augment the compilation
      environment by providing additional include files and paths and libraries and
      paths.


   .. method:: CCompiler.library_dir_option(dir)

      Return the compiler option to add *dir* to the list of directories searched for
      libraries.


   .. method:: CCompiler.library_option(lib)

      Return the compiler option to add *dir* to the list of libraries linked into the
      shared library or executable.


   .. method:: CCompiler.runtime_library_dir_option(dir)

      Return the compiler option to add *dir* to the list of directories searched for
      runtime libraries.


   .. method:: CCompiler.set_executables(**args)

      Define the executables (and options for them) that will be run to perform the
      various stages of compilation.  The exact set of executables that may be
      specified here depends on the compiler class (via the 'executables' class
      attribute), but most will have:

      +--------------+------------------------------------------+
      | attribute    | description                              |
      +==============+==========================================+
      | *compiler*   | the C/C++ compiler                       |
      +--------------+------------------------------------------+
      | *linker_so*  | linker used to create shared objects and |
      |              | libraries                                |
      +--------------+------------------------------------------+
      | *linker_exe* | linker used to create binary executables |
      +--------------+------------------------------------------+
      | *archiver*   | static library creator                   |
      +--------------+------------------------------------------+

      On platforms with a command line (Unix, DOS/Windows), each of these is a string
      that will be split into executable name and (optional) list of arguments.
      (Splitting the string is done similarly to how Unix shells operate: words are
      delimited by spaces, but quotes and backslashes can override this.  See
      :func:`packaging.util.split_quoted`.)

   The following methods invoke stages in the build process.


   .. method:: CCompiler.compile(sources, output_dir=None, macros=None, include_dirs=None, debug=0, extra_preargs=None, extra_postargs=None, depends=None)

      Compile one or more source files. Generates object files (e.g. transforms a
      :file:`.c` file to a :file:`.o` file.)

      *sources* must be a list of filenames, most likely C/C++ files, but in reality
      anything that can be handled by a particular compiler and compiler class (e.g.
      an ``'msvc'`` compiler can handle resource files in *sources*).  Return a list of
      object filenames, one per source filename in *sources*.  Depending on the
      implementation, not all source files will necessarily be compiled, but all
      corresponding object filenames will be returned.

      If *output_dir* is given, object files will be put under it, while retaining
      their original path component.  That is, :file:`foo/bar.c` normally compiles to
      :file:`foo/bar.o` (for a Unix implementation); if *output_dir* is *build*, then
      it would compile to :file:`build/foo/bar.o`.

      *macros*, if given, must be a list of macro definitions.  A macro definition is
      either a ``(name, value)`` 2-tuple or a ``(name,)`` 1-tuple. The former defines
      a macro; if the value is ``None``, the macro is defined without an explicit
      value.  The 1-tuple case undefines a macro.  Later
      definitions/redefinitions/undefinitions take precedence.

      *include_dirs*, if given, must be a list of strings, the directories to add to
      the default include file search path for this compilation only.

      *debug* is a boolean; if true, the compiler will be instructed to output debug
      symbols in (or alongside) the object file(s).

      *extra_preargs* and *extra_postargs* are implementation-dependent. On platforms
      that have the notion of a command line (e.g. Unix, DOS/Windows), they are most
      likely lists of strings: extra command-line arguments to prepend/append to the
      compiler command line.  On other platforms, consult the implementation class
      documentation.  In any event, they are intended as an escape hatch for those
      occasions when the abstract compiler framework doesn't cut the mustard.

      *depends*, if given, is a list of filenames that all targets depend on.  If a
      source file is older than any file in depends, then the source file will be
      recompiled.  This supports dependency tracking, but only at a coarse
      granularity.

      Raises :exc:`CompileError` on failure.


   .. method:: CCompiler.create_static_lib(objects, output_libname, output_dir=None, debug=0, target_lang=None)

      Link a bunch of stuff together to create a static library file. The "bunch of
      stuff" consists of the list of object files supplied as *objects*, the extra
      object files supplied to :meth:`add_link_object` and/or
      :meth:`set_link_objects`, the libraries supplied to :meth:`add_library` and/or
      :meth:`set_libraries`, and the libraries supplied as *libraries* (if any).

      *output_libname* should be a library name, not a filename; the filename will be
      inferred from the library name.  *output_dir* is the directory where the library
      file will be put. XXX defaults to what?

      *debug* is a boolean; if true, debugging information will be included in the
      library (note that on most platforms, it is the compile step where this matters:
      the *debug* flag is included here just for consistency).

      *target_lang* is the target language for which the given objects are being
      compiled. This allows specific linkage time treatment of certain languages.

      Raises :exc:`LibError` on failure.


   .. method:: CCompiler.link(target_desc, objects, output_filename, output_dir=None, libraries=None, library_dirs=None, runtime_library_dirs=None, export_symbols=None, debug=0, extra_preargs=None, extra_postargs=None, build_temp=None, target_lang=None)

      Link a bunch of stuff together to create an executable or shared library file.

      The "bunch of stuff" consists of the list of object files supplied as *objects*.
      *output_filename* should be a filename.  If *output_dir* is supplied,
      *output_filename* is relative to it (i.e. *output_filename* can provide
      directory components if needed).

      *libraries* is a list of libraries to link against.  These are library names,
      not filenames, since they're translated into filenames in a platform-specific
      way (e.g. *foo* becomes :file:`libfoo.a` on Unix and :file:`foo.lib` on
      DOS/Windows).  However, they can include a directory component, which means the
      linker will look in that specific directory rather than searching all the normal
      locations.

      *library_dirs*, if supplied, should be a list of directories to search for
      libraries that were specified as bare library names (i.e. no directory
      component).  These are on top of the system default and those supplied to
      :meth:`add_library_dir` and/or :meth:`set_library_dirs`.  *runtime_library_dirs*
      is a list of directories that will be embedded into the shared library and used
      to search for other shared libraries that \*it\* depends on at run-time.  (This
      may only be relevant on Unix.)

      *export_symbols* is a list of symbols that the shared library will export.
      (This appears to be relevant only on Windows.)

      *debug* is as for :meth:`compile` and :meth:`create_static_lib`, with the
      slight distinction that it actually matters on most platforms (as opposed to
      :meth:`create_static_lib`, which includes a *debug* flag mostly for form's
      sake).

      *extra_preargs* and *extra_postargs* are as for :meth:`compile` (except of
      course that they supply command-line arguments for the particular linker being
      used).

      *target_lang* is the target language for which the given objects are being
      compiled. This allows specific linkage time treatment of certain languages.

      Raises :exc:`LinkError` on failure.


   .. method:: CCompiler.link_executable(objects, output_progname, output_dir=None, libraries=None, library_dirs=None, runtime_library_dirs=None, debug=0, extra_preargs=None, extra_postargs=None, target_lang=None)

      Link an executable.  *output_progname* is the name of the file executable, while
      *objects* are a list of object filenames to link in. Other arguments are as for
      the :meth:`link` method.


   .. method:: CCompiler.link_shared_lib(objects, output_libname, output_dir=None, libraries=None, library_dirs=None, runtime_library_dirs=None, export_symbols=None, debug=0, extra_preargs=None, extra_postargs=None, build_temp=None, target_lang=None)

      Link a shared library. *output_libname* is the name of the output library,
      while *objects* is a list of object filenames to link in. Other arguments are
      as for the :meth:`link` method.


   .. method:: CCompiler.link_shared_object(objects, output_filename, output_dir=None, libraries=None, library_dirs=None, runtime_library_dirs=None, export_symbols=None, debug=0, extra_preargs=None, extra_postargs=None, build_temp=None, target_lang=None)

      Link a shared object. *output_filename* is the name of the shared object that
      will be created, while *objects* is a list of object filenames to link in.
      Other arguments are as for the :meth:`link` method.


   .. method:: CCompiler.preprocess(source, output_file=None, macros=None, include_dirs=None, extra_preargs=None, extra_postargs=None)

      Preprocess a single C/C++ source file, named in *source*. Output will be written
      to file named *output_file*, or *stdout* if *output_file* not supplied.
      *macros* is a list of macro definitions as for :meth:`compile`, which will
      augment the macros set with :meth:`define_macro` and :meth:`undefine_macro`.
      *include_dirs* is a list of directory names that will be added to the default
      list, in the same way as :meth:`add_include_dir`.

      Raises :exc:`PreprocessError` on failure.

   The following utility methods are defined by the :class:`CCompiler` class, for
   use by the various concrete subclasses.


   .. method:: CCompiler.executable_filename(basename, strip_dir=0, output_dir='')

      Returns the filename of the executable for the given *basename*.  Typically for
      non-Windows platforms this is the same as the basename, while Windows will get
      a :file:`.exe` added.


   .. method:: CCompiler.library_filename(libname, lib_type='static', strip_dir=0, output_dir='')

      Returns the filename for the given library name on the current platform. On Unix
      a library with *lib_type* of ``'static'`` will typically be of the form
      :file:`liblibname.a`, while a *lib_type* of ``'dynamic'`` will be of the form
      :file:`liblibname.so`.


   .. method:: CCompiler.object_filenames(source_filenames, strip_dir=0, output_dir='')

      Returns the name of the object files for the given source files.
      *source_filenames* should be a list of filenames.


   .. method:: CCompiler.shared_object_filename(basename, strip_dir=0, output_dir='')

      Returns the name of a shared object file for the given file name *basename*.


   .. method:: CCompiler.execute(func, args, msg=None, level=1)

      Invokes :func:`packaging.util.execute` This method invokes a Python function
      *func* with the given arguments *args*, after logging and taking into account
      the *dry_run* flag. XXX see also.


   .. method:: CCompiler.spawn(cmd)

      Invokes :func:`packaging.util.spawn`. This invokes an external process to run
      the given command. XXX see also.


   .. method:: CCompiler.mkpath(name, mode=511)

      Invokes :func:`packaging.dir_util.mkpath`. This creates a directory and any
      missing ancestor directories. XXX see also.


   .. method:: CCompiler.move_file(src, dst)

      Invokes :meth:`packaging.file_util.move_file`. Renames *src* to *dst*.  XXX see
      also.


:mod:`packaging.compiler.extension` --- The Extension class
===========================================================

.. module:: packaging.compiler.extension
   :synopsis: Class used to represent C/C++ extension modules.


This module provides the :class:`Extension` class, used to represent C/C++
extension modules.

.. class:: Extension

   The Extension class describes a single C or C++ extension module.  It accepts
   the following keyword arguments in its constructor:

   +------------------------+--------------------------------+---------------------------+
   | argument name          | value                          | type                      |
   +========================+================================+===========================+
   | *name*                 | the full name of the           | string                    |
   |                        | extension, including any       |                           |
   |                        | packages --- i.e. *not* a      |                           |
   |                        | filename or pathname, but      |                           |
   |                        | Python dotted name             |                           |
   +------------------------+--------------------------------+---------------------------+
   | *sources*              | list of source filenames,      | list of strings           |
   |                        | relative to the distribution   |                           |
   |                        | root (where the setup script   |                           |
   |                        | lives), in Unix form (slash-   |                           |
   |                        | separated) for portability.    |                           |
   |                        | Source files may be C, C++,    |                           |
   |                        | SWIG (.i), platform-specific   |                           |
   |                        | resource files, or whatever    |                           |
   |                        | else is recognized by the      |                           |
   |                        | :command:`build_ext` command   |                           |
   |                        | as source for a Python         |                           |
   |                        | extension.                     |                           |
   +------------------------+--------------------------------+---------------------------+
   | *include_dirs*         | list of directories to search  | list of strings           |
   |                        | for C/C++ header files (in     |                           |
   |                        | Unix form for portability)     |                           |
   +------------------------+--------------------------------+---------------------------+
   | *define_macros*        | list of macros to define; each | list of tuples            |
   |                        | macro is defined using a       |                           |
   |                        | 2-tuple ``(name, value)``,     |                           |
   |                        | where *value* is               |                           |
   |                        | either the string to define it |                           |
   |                        | to or ``None`` to define it    |                           |
   |                        | without a particular value     |                           |
   |                        | (equivalent of ``#define FOO`` |                           |
   |                        | in source or :option:`-DFOO`   |                           |
   |                        | on Unix C compiler command     |                           |
   |                        | line)                          |                           |
   +------------------------+--------------------------------+---------------------------+
   | *undef_macros*         | list of macros to undefine     | list of strings           |
   |                        | explicitly                     |                           |
   +------------------------+--------------------------------+---------------------------+
   | *library_dirs*         | list of directories to search  | list of strings           |
   |                        | for C/C++ libraries at link    |                           |
   |                        | time                           |                           |
   +------------------------+--------------------------------+---------------------------+
   | *libraries*            | list of library names (not     | list of strings           |
   |                        | filenames or paths) to link    |                           |
   |                        | against                        |                           |
   +------------------------+--------------------------------+---------------------------+
   | *runtime_library_dirs* | list of directories to search  | list of strings           |
   |                        | for C/C++ libraries at run     |                           |
   |                        | time (for shared extensions,   |                           |
   |                        | this is when the extension is  |                           |
   |                        | loaded)                        |                           |
   +------------------------+--------------------------------+---------------------------+
   | *extra_objects*        | list of extra files to link    | list of strings           |
   |                        | with (e.g. object files not    |                           |
   |                        | implied by 'sources', static   |                           |
   |                        | library that must be           |                           |
   |                        | explicitly specified, binary   |                           |
   |                        | resource files, etc.)          |                           |
   +------------------------+--------------------------------+---------------------------+
   | *extra_compile_args*   | any extra platform- and        | list of strings           |
   |                        | compiler-specific information  |                           |
   |                        | to use when compiling the      |                           |
   |                        | source files in 'sources'. For |                           |
   |                        | platforms and compilers where  |                           |
   |                        | a command line makes sense,    |                           |
   |                        | this is typically a list of    |                           |
   |                        | command-line arguments, but    |                           |
   |                        | for other platforms it could   |                           |
   |                        | be anything.                   |                           |
   +------------------------+--------------------------------+---------------------------+
   | *extra_link_args*      | any extra platform- and        | list of strings           |
   |                        | compiler-specific information  |                           |
   |                        | to use when linking object     |                           |
   |                        | files together to create the   |                           |
   |                        | extension (or to create a new  |                           |
   |                        | static Python interpreter).    |                           |
   |                        | Similar interpretation as for  |                           |
   |                        | 'extra_compile_args'.          |                           |
   +------------------------+--------------------------------+---------------------------+
   | *export_symbols*       | list of symbols to be exported | list of strings           |
   |                        | from a shared extension. Not   |                           |
   |                        | used on all platforms, and not |                           |
   |                        | generally necessary for Python |                           |
   |                        | extensions, which typically    |                           |
   |                        | export exactly one symbol:     |                           |
   |                        | ``init`` + extension_name.     |                           |
   +------------------------+--------------------------------+---------------------------+
   | *depends*              | list of files that the         | list of strings           |
   |                        | extension depends on           |                           |
   +------------------------+--------------------------------+---------------------------+
   | *language*             | extension language (i.e.       | string                    |
   |                        | ``'c'``, ``'c++'``,            |                           |
   |                        | ``'objc'``). Will be detected  |                           |
   |                        | from the source extensions if  |                           |
   |                        | not provided.                  |                           |
   +------------------------+--------------------------------+---------------------------+
   | *optional*             | specifies that a build failure | boolean                   |
   |                        | in the extension should not    |                           |
   |                        | abort the build process, but   |                           |
   |                        | simply skip the extension.     |                           |
   +------------------------+--------------------------------+---------------------------+

To distribute extension modules that live in a package (e.g. ``package.ext``),
you need to create a :file:`{package}/__init__.py` file to let Python recognize
and import your module.
