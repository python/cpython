Example Python extension for Windows NT
=======================================

This directory contains everything you need to build a Python
extension module using Microsoft VC++ ("Developer Studio") version 4.x
or 5.x, except for the Python distribution.  It has been tested with
VC++ 4.2 on Python 1.5a3, and with VC++ 5.0 on Python 1.5b2.

The "example_nt" subdirectory should be an immediate subdirectory of
the Python source directory -- a direct sibling of Include and PC, in
particular, which are referenced as "..\Include" and "..\PC".  In
other words, it should *not* be used "as is".  Copy or move it up one
level or you will regret it!  (This is done to keep all the PC
specific files inside the PC subdirectory of the distribution, where
they belong.)

When using the VC++ 4.x project (makefile), it is assumed that the
build results of Python are in the directory ..\vc40.  In particular,
the python15.lib file is referred to as "..\vc40\python15.lib".  If
you have problems with this file, the best thing to do is to delete it
from the project and add it again.

When using the VC++ 5.x project (workspace), the build results of
Python are assumed to be in ..\PCbuild.  Since the provided VC++ 5.x
project and workspace files have a different structure (to support
separate "release" and "debug" builds), the example project and
workspace match this structure.

In order to use the example project from VC++ 4.x, use the "File->Open
Workspace..." dialog (*not* the "File->Open..." dialog!).  Change the
pattern to "*.mak" and select the file "example.mak".  Now choose
"File->Save All" and the othe project files will be created.

From VC+ 5.x, do the same except don't change the pattern, and select
the example.dsw workspace file.

In order to check that everything is set up right, try building:
choose "Build->Build example.dll".  This creates all intermediate and
result files in a subdirectory which is called either Debug or Release
depending on which configuration you have chosen.

Once the build has succeeded, test the resulting DLL.  In a DOS
command window, chdir to that directory.  You should now be able to
repeat the following session "(C>" is the DOS prompt, ">>>" is the
Python prompt):

	C> ..\..\vc40\python.exe
	>>> import example
	>>> example.foo()
	Hello, world
	>>>

When using VC++ 5.x, issue these commands:

	C> ..\..\PCbuild\Release\python.exe
	>>> import example
	>>> example.foo()
	Hello, world
	>>>


Creating the project
--------------------

There are two ways to use this example to create a project for your
own module.  First, choose a name ("spam" is always a winner :-) and
create a directory for it.  Copy your C sources into it.  Note that
the module source file name does not necessarily have to match the
module name, but the "init" function name should match the module name
-- i.e. you can only import a module "spam" if its init function is
called "initspam()", and it should call Py_InitModule with the string
"spam" as its first argument.  By convention, it lives in a file
called "spam.c" or "spammodule.c".  The output file should be called
"spam.dll" or "spam.pyd" (the latter is supported to avoid confusion
with a system library "spam.dll" to which your module could be a
Python interface).

Now your options are:

1) Clone example.mak.  Start by copying example_nt\example.mak to
spam\spam.mak.  Do a global edit on spam.mak, replacing all
occurrences of the string "example" by "spam", and all occurrences of
"DEP_CPP_EXAMP" by something like "DEP_CPP_SPAM".  You can now use
this makefile to create a project file by opening it as a workspace
(you have to change the pattern to *.mak first).  (When using VC++
5.x, you can clone example.dsp and example.dsw in a similar way.)

2) Create a brand new project; instructions are below.

In both cases, copy example_nt\example.def to spam\spam.def, and edit
spam\spam.def so its second line contains the string "initspam".  If
you created a new project yourself, add the file spam.def to the
project now.  (This is an annoying little file with only two lines.
An alternative approach is to forget about the .def file, and add the
option "/export:initspam" somewhere to the Link settings, by manually
editing the "Project Options" box).

You are now all set to build your extension, unless it requires other
external libraries, include files, etc.  See Python's Extending and
Embedding manual for instructions on how to write an extension.


Creating a brand new project
----------------------------

If you don't feel comfortable with editing Makefiles or project and
workspace files, you can create a brand new project from scratch
easily.

Use the "File->New..." dialog to create a new Project Workspace.
Select Dynamic-Link Library, enter the name ("spam"), and make sure
the "Location" is set to the spam directory you have created (which
should be a direct subdirectory of the Python build tree).  Select
Win32 as the platform (in my version, this is the only choice).  Click
"Create".

Now open the "Build->Settings..." dialog.  (Impressive, isn't it?  :-)
You only need to change a few settings.  Make sure you have both the
Debug and the Release configuration selected when you make the first
change.  Select the "C/C++" tab.  Choose the "Preprocessor" category
in the popup menu at the top.  Type the following text in the entry
box labeled "Addditional include directories:"

	..\Include,..\PC

Next, for both configurations, select the "Link" tab, choose the
"General" category, and add "python15.lib" to the end of the
"Object/library modules" box.

Then, separately for the Release and Debug configurations, choose the
"Input" category in the Link tab, and enter "..\PCbuild\Release" or
"..\PCbuild\Debug", respectively, in the "Additional library path"
box.

Finally, you must change the run-time library.  This must also be done
separately for the Release and Debug configurations.  Choose the "Code
Generation" category in the C/C++ tab.  In the box labeled "Use
run-time library", choose "Multithreaded DLL" for the Release
configuration, and "Debug Multithreaded DLL" for the Debug
configuration.  That's all.

You should now first create the file spam.def as instructed in the
previous section.

Now chose the "Insert->Files into Project..." dialog.  Set the pattern
to *.* and select both spam.c and spam.def and click OK.  (Inserting
them one by one is fine too.)

