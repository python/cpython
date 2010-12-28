.. highlightlang:: c


.. _building-on-windows:

****************************************
Building C and C++ Extensions on Windows
****************************************

This chapter briefly explains how to create a Windows extension module for
Python using Microsoft Visual C++, and follows with more detailed background
information on how it works.  The explanatory material is useful for both the
Windows programmer learning to build Python extensions and the Unix programmer
interested in producing software which can be successfully built on both Unix
and Windows.

Module authors are encouraged to use the distutils approach for building
extension modules, instead of the one described in this section. You will still
need the C compiler that was used to build Python; typically Microsoft Visual
C++.

.. note::

   This chapter mentions a number of filenames that include an encoded Python
   version number.  These filenames are represented with the version number shown
   as ``XY``; in practice, ``'X'`` will be the major version number and ``'Y'``
   will be the minor version number of the Python release you're working with.  For
   example, if you are using Python 2.2.1, ``XY`` will actually be ``22``.


.. _win-cookbook:

A Cookbook Approach
===================

There are two approaches to building extension modules on Windows, just as there
are on Unix: use the :mod:`distutils` package to control the build process, or
do things manually.  The distutils approach works well for most extensions;
documentation on using :mod:`distutils` to build and package extension modules
is available in :ref:`distutils-index`.  This section describes the manual
approach to building Python extensions written in C or C++.

To build extensions using these instructions, you need to have a copy of the
Python sources of the same version as your installed Python. You will need
Microsoft Visual C++ "Developer Studio"; project files are supplied for VC++
version 7.1, but you can use older versions of VC++.  Notice that you should use
the same version of VC++that was used to build Python itself. The example files
described here are distributed with the Python sources in the
:file:`PC\\example_nt\\` directory.

#. **Copy the example files** ---  The :file:`example_nt` directory is a
   subdirectory of the :file:`PC` directory, in order to keep all the PC-specific
   files under the same directory in the source distribution.  However, the
   :file:`example_nt` directory can't actually be used from this location.  You
   first need to copy or move it up one level, so that :file:`example_nt` is a
   sibling of the :file:`PC` and :file:`Include` directories.  Do all your work
   from within this new location.

#. **Open the project** ---  From VC++, use the :menuselection:`File --> Open
   Solution` dialog (not :menuselection:`File --> Open`!).  Navigate to and select
   the file :file:`example.sln`, in the *copy* of the :file:`example_nt` directory
   you made above.  Click Open.

#. **Build the example DLL** ---  In order to check that everything is set up
   right, try building:

#. Select a configuration.  This step is optional.  Choose
   :menuselection:`Build --> Configuration Manager --> Active Solution Configuration`
   and select either :guilabel:`Release`  or :guilabel:`Debug`.  If you skip this
   step, VC++ will use the Debug configuration by default.

#. Build the DLL.  Choose :menuselection:`Build --> Build Solution`.  This
   creates all intermediate and result files in a subdirectory called either
   :file:`Debug` or :file:`Release`, depending on which configuration you selected
   in the preceding step.

#. **Testing the debug-mode DLL** ---  Once the Debug build has succeeded, bring
   up a DOS box, and change to the :file:`example_nt\\Debug` directory.  You should
   now be able to repeat the following session (``C>`` is the DOS prompt, ``>>>``
   is the Python prompt; note that build information and various debug output from
   Python may not match this screen dump exactly)::

      C>..\..\PCbuild\python_d
      Adding parser accelerators ...
      Done.
      Python 2.2 (#28, Dec 19 2001, 23:26:37) [MSC 32 bit (Intel)] on win32
      Type "copyright", "credits" or "license" for more information.
      >>> import example
      [4897 refs]
      >>> example.foo()
      Hello, world
      [4903 refs]
      >>>

   Congratulations!  You've successfully built your first Python extension module.

#. **Creating your own project** ---  Choose a name and create a directory for
   it.  Copy your C sources into it.  Note that the module source file name does
   not necessarily have to match the module name, but the name of the
   initialization function should match the module name --- you can only import a
   module :mod:`spam` if its initialization function is called :c:func:`initspam`,
   and it should call :c:func:`Py_InitModule` with the string ``"spam"`` as its
   first argument (use the minimal :file:`example.c` in this directory as a guide).
   By convention, it lives in a file called :file:`spam.c` or :file:`spammodule.c`.
   The output file should be called :file:`spam.pyd` (in Release mode) or
   :file:`spam_d.pyd` (in Debug mode). The extension :file:`.pyd` was chosen
   to avoid confusion with a system library :file:`spam.dll` to which your module
   could be a Python interface.

   Now your options are:

#. Copy :file:`example.sln` and :file:`example.vcproj`, rename them to
   :file:`spam.\*`, and edit them by hand, or

#. Create a brand new project; instructions are below.

   In either case, copy :file:`example_nt\\example.def` to :file:`spam\\spam.def`,
   and edit the new :file:`spam.def` so its second line contains the string
   '``initspam``'.  If you created a new project yourself, add the file
   :file:`spam.def` to the project now.  (This is an annoying little file with only
   two lines.  An alternative approach is to forget about the :file:`.def` file,
   and add the option :option:`/export:initspam` somewhere to the Link settings, by
   manually editing the setting in Project Properties dialog).

#. **Creating a brand new project** ---  Use the :menuselection:`File --> New
   --> Project` dialog to create a new Project Workspace.  Select :guilabel:`Visual
   C++ Projects/Win32/ Win32 Project`, enter the name (``spam``), and make sure the
   Location is set to parent of the :file:`spam` directory you have created (which
   should be a direct subdirectory of the Python build tree, a sibling of
   :file:`Include` and :file:`PC`).  Select Win32 as the platform (in my version,
   this is the only choice).  Make sure the Create new workspace radio button is
   selected.  Click OK.

   You should now create the file :file:`spam.def` as instructed in the previous
   section. Add the source files to the project, using :menuselection:`Project -->
   Add Existing Item`. Set the pattern to ``*.*`` and select both :file:`spam.c`
   and :file:`spam.def` and click OK.  (Inserting them one by one is fine too.)

   Now open the :menuselection:`Project --> spam properties` dialog. You only need
   to change a few settings.  Make sure :guilabel:`All Configurations` is selected
   from the :guilabel:`Settings for:` dropdown list.  Select the C/C++ tab.  Choose
   the General category in the popup menu at the top.  Type the following text in
   the entry box labeled :guilabel:`Additional Include Directories`::

      ..\Include,..\PC

   Then, choose the General category in the Linker tab, and enter ::

      ..\PCbuild

   in the text box labelled :guilabel:`Additional library Directories`.

   Now you need to add some mode-specific settings:

   Select :guilabel:`Release` in the :guilabel:`Configuration` dropdown list.
   Choose the :guilabel:`Link` tab, choose the :guilabel:`Input` category, and
   append ``pythonXY.lib`` to the list in the :guilabel:`Additional Dependencies`
   box.

   Select :guilabel:`Debug` in the :guilabel:`Configuration` dropdown list, and
   append ``pythonXY_d.lib`` to the list in the :guilabel:`Additional Dependencies`
   box.  Then click the C/C++ tab, select :guilabel:`Code Generation`, and select
   :guilabel:`Multi-threaded Debug DLL` from the :guilabel:`Runtime library`
   dropdown list.

   Select :guilabel:`Release` again from the :guilabel:`Configuration` dropdown
   list.  Select :guilabel:`Multi-threaded DLL` from the :guilabel:`Runtime
   library` dropdown list.

If your module creates a new type, you may have trouble with this line::

   PyVarObject_HEAD_INIT(&PyType_Type, 0)

Static type object initializers in extension modules may cause
compiles to fail with an error message like "initializer not a
constant".  This shows up when building DLL under MSVC.  Change it to::

   PyVarObject_HEAD_INIT(NULL, 0)

and add the following to the module initialization function::

   if (PyType_Ready(&MyObject_Type) < 0)
        return NULL;


.. _dynamic-linking:

Differences Between Unix and Windows
====================================

.. sectionauthor:: Chris Phoenix <cphoenix@best.com>


Unix and Windows use completely different paradigms for run-time loading of
code.  Before you try to build a module that can be dynamically loaded, be aware
of how your system works.

In Unix, a shared object (:file:`.so`) file contains code to be used by the
program, and also the names of functions and data that it expects to find in the
program.  When the file is joined to the program, all references to those
functions and data in the file's code are changed to point to the actual
locations in the program where the functions and data are placed in memory.
This is basically a link operation.

In Windows, a dynamic-link library (:file:`.dll`) file has no dangling
references.  Instead, an access to functions or data goes through a lookup
table.  So the DLL code does not have to be fixed up at runtime to refer to the
program's memory; instead, the code already uses the DLL's lookup table, and the
lookup table is modified at runtime to point to the functions and data.

In Unix, there is only one type of library file (:file:`.a`) which contains code
from several object files (:file:`.o`).  During the link step to create a shared
object file (:file:`.so`), the linker may find that it doesn't know where an
identifier is defined.  The linker will look for it in the object files in the
libraries; if it finds it, it will include all the code from that object file.

In Windows, there are two types of library, a static library and an import
library (both called :file:`.lib`).  A static library is like a Unix :file:`.a`
file; it contains code to be included as necessary. An import library is
basically used only to reassure the linker that a certain identifier is legal,
and will be present in the program when the DLL is loaded.  So the linker uses
the information from the import library to build the lookup table for using
identifiers that are not included in the DLL.  When an application or a DLL is
linked, an import library may be generated, which will need to be used for all
future DLLs that depend on the symbols in the application or DLL.

Suppose you are building two dynamic-load modules, B and C, which should share
another block of code A.  On Unix, you would *not* pass :file:`A.a` to the
linker for :file:`B.so` and :file:`C.so`; that would cause it to be included
twice, so that B and C would each have their own copy.  In Windows, building
:file:`A.dll` will also build :file:`A.lib`.  You *do* pass :file:`A.lib` to the
linker for B and C.  :file:`A.lib` does not contain code; it just contains
information which will be used at runtime to access A's code.

In Windows, using an import library is sort of like using ``import spam``; it
gives you access to spam's names, but does not create a separate copy.  On Unix,
linking with a library is more like ``from spam import *``; it does create a
separate copy.


.. _win-dlls:

Using DLLs in Practice
======================

.. sectionauthor:: Chris Phoenix <cphoenix@best.com>


Windows Python is built in Microsoft Visual C++; using other compilers may or
may not work (though Borland seems to).  The rest of this section is MSVC++
specific.

When creating DLLs in Windows, you must pass :file:`pythonXY.lib` to the linker.
To build two DLLs, spam and ni (which uses C functions found in spam), you could
use these commands::

   cl /LD /I/python/include spam.c ../libs/pythonXY.lib
   cl /LD /I/python/include ni.c spam.lib ../libs/pythonXY.lib

The first command created three files: :file:`spam.obj`, :file:`spam.dll` and
:file:`spam.lib`.  :file:`Spam.dll` does not contain any Python functions (such
as :c:func:`PyArg_ParseTuple`), but it does know how to find the Python code
thanks to :file:`pythonXY.lib`.

The second command created :file:`ni.dll` (and :file:`.obj` and :file:`.lib`),
which knows how to find the necessary functions from spam, and also from the
Python executable.

Not every identifier is exported to the lookup table.  If you want any other
modules (including Python) to be able to see your identifiers, you have to say
``_declspec(dllexport)``, as in ``void _declspec(dllexport) initspam(void)`` or
``PyObject _declspec(dllexport) *NiGetSpamData(void)``.

Developer Studio will throw in a lot of import libraries that you do not really
need, adding about 100K to your executable.  To get rid of them, use the Project
Settings dialog, Link tab, to specify *ignore default libraries*.  Add the
correct :file:`msvcrtxx.lib` to the list of libraries.

