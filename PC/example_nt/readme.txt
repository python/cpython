*** WARNING -- THIS DIRECTORY IS OUT OF DATE.
*** It has not been updated since Python 1.4beta3.


Example Python extension for Windows NT
=======================================

This directory contains everything you need to build a Python
extension module using Microsoft VC++ 4.x ("Developer Studio"), except
for the Python distribution.  It has only been tested with version
4.0, but should work with higher versions.

The "example" subdirectory should be an immediate subdirectory of the
Python source directory -- a direct sibling of Include and PC, in
particular, which are referenced as "..\Include" and "..\PC".
In other words, it should *not* be used "as is".  Copy or move it up
one level or you will regret it!  (This is done to keep all the PC
specific files inside the PC subdirectory of the distribution, where
they belong.)

It is also assumed that the build results of Python are in the
directory ..\vc40.  In particular, the python14.lib file is referred
to as "..\vc40\python14.lib".

In order to use the example project from Developer Studio, use the
"File->Open Workspace..." dialog (*not* the "File->Open..." dialog!).
Change the pattern to "*.mak" and select the file "example.mak".  Now
choose "File->Save All" and the othe project files will be created.

In order to check that everything is set up right, try building:
choose "Build->Build example.dll".  This creates all intermediate and
result files in a subdirectory which is called either Debug or Release
depending on which configuration you have chosen (as distributed,
Debug is selected as the default configuration).

Once the build has succeeded, test the resulting DLL.  In a DOS
command window, chdir to that directory.  You should now be able to
repeat the following session "(C>" is the DOS prompt, ">>>" is the
Python prompt):

	C> ..\..\vc40\python.exe
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

1) Clone example.mak.  Start by copying example\example.mak to
spam\spam.mak.  Do a global edit on spam.mak, replacing all
occurrences of the string "example" by "spam", and all occurrences of
"DEP_CPP_EXAMP" by something like "DEP_CPP_SPAM".  You can now use
this makefile to create a project file by opening it as a workspace
(you have to change the pattern to *.mak first).

2) Create a brand new project; instructions are below.

In both cases, copy example\example.def to spam\spam.def, and edit
spam\spam.def so its second line contains the string "initspam".
If you created a new project yourself, add the file spam.def to the
project now.

You are now all set to build your extension, unless it requires other
external libraries, include files, etc.  See Python's Extending and
Embedding manual for instructions on how to write an extension.


Creating a brand new project
----------------------------

If you don't feel comfortable with editing Makefiles, you can create a
brand new project from scratch easily.

Use the "File->New..." dialog to create a new Project Workspace.
Select Dynamic-Link Library, enter the name ("spam"), and make sure
the "Location" is set to the spam directory you have created (which
should be a direct subdirectory of the Python build tree).  Select
Win32 as the platform (in my version, this is the only choice).  Click
"Create".

Now open the "Build->Settings..." dialog.  (Impressive, isn't it?  :-)
You only need to change a few settings.  Make sure you have both the
Debug and the Release configuration selected when you make these
changes.  Select the "C/C++" tab.  Choose the "Preprocessor" category
in the popup menu at the top.  Type the following text in the entry
box labeled "Addditional include directories:"

	..\Include,..\PC

You should now first create the file spam.def as instructed in the
previous section.

Now chose the "Insert->Files into Project..." dialog.  Set the pattern
to *.* and select both spam.c and spam.def and click OK.  (Inserting
them one by one is fine too.)  Using the same dialog, choose the file
..\vc40\python14.lib and insert it into the project.
