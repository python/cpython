Example Python extension for Windows NT
=======================================

This directory contains everything needed (except for the Python
distribution!) to build a Python extension module using Microsoft VC++
("Developer Studio") version 7.1.  It has been tested with VC++ 7.1 on 
Python 2.4.  You can also use earlier versions of VC to build Python 
extensions, but the sample VC project file (example.dsw in this directory) 
is in VC 7.1 format. Notice that you need to use the same compiler version
that was used to build Python itself.

COPY THIS DIRECTORY!
--------------------
This "example_nt" directory is a subdirectory of the PC directory, in order
to keep all the PC-specific files under the same directory.  However, the
example_nt directory can't actually be used from this location.  You first
need to copy or move it up one level, so that example_nt is a direct
sibling of the PC\ and Include\ directories.  Do all your work from within
this new location -- sorry, but you'll be sorry if you don't.

OPEN THE PROJECT
----------------
From VC 7.1, use the
    File -> Open Solution...
dialog (*not* the "File -> Open..." dialog!).  Navigate to and select the
file "example.sln", in the *copy* of the example_nt directory you made
above.
Click Open.

BUILD THE EXAMPLE DLL
---------------------
In order to check that everything is set up right, try building:

1. Select a configuration.  This step is optional.  Do
       Build -> Configuration Manager... -> Active Solution Configuration
   and select either "Release" or "Debug".
   If you skip this step, you'll use the Debug configuration by default.

2. Build the DLL.  Do
       Build -> Build Solution
   This creates all intermediate and result files in a subdirectory which
   is called either Debug or Release, depending on which configuration you
   picked in the preceding step.

TESTING THE DEBUG-MODE DLL
--------------------------
Once the Debug build has succeeded, bring up a DOS box, and cd to
example_nt\Debug.  You should now be able to repeat the following session
("C>" is the DOS prompt, ">>>" is the Python prompt) (note that various
debug output from Python may not match this screen dump exactly):

    C>..\..\PCbuild\python_d
    Adding parser accelerators ...
    Done.
    Python 2.2c1+ (#28, Dec 14 2001, 18:06:39) [MSC 32 bit (Intel)] on win32
    Type "help", "copyright", "credits" or "license" for more information.
    >>> import example
    [7052 refs]
    >>> example.foo()
    Hello, world
    [7052 refs]
    >>>

TESTING THE RELEASE-MODE DLL
----------------------------
Once the Release build has succeeded, bring up a DOS box, and cd to
example_nt\Release.  You should now be able to repeat the following session
("C>" is the DOS prompt, ">>>" is the Python prompt):

    C>..\..\PCbuild\python
    Python 2.2c1+ (#28, Dec 14 2001, 18:06:04) [MSC 32 bit (Intel)] on win32
    Type "help", "copyright", "credits" or "license" for more information.
    >>> import example
    >>> example.foo()
    Hello, world
    >>>

Congratulations!  You've successfully built your first Python extension
module.

CREATING YOUR OWN PROJECT
-------------------------
Choose a name ("spam" is always a winner :-) and create a directory for
it.  Copy your C sources into it.  Note that the module source file name
does not necessarily have to match the module name, but the "init" function
name should match the module name -- i.e. you can only import a module
"spam" if its init function is called "initspam()", and it should call
Py_InitModule with the string "spam" as its first argument (use the minimal
example.c in this directory as a guide).  By convention, it lives in a file
called "spam.c" or "spammodule.c".  The output file should be called
"spam.dll" or "spam.pyd" (the latter is supported to avoid confusion with a
system library "spam.dll" to which your module could be a Python interface)
in Release mode, or spam_d.dll or spam_d.pyd in Debug mode.

Now your options are:

1) Copy example.sln and example.vcproj, rename them to spam.*, and edit them
by hand.

or

2) Create a brand new project; instructions are below.

In either case, copy example_nt\example.def to spam\spam.def, and edit the
new spam.def so its second line contains the string "initspam".  If you
created a new project yourself, add the file spam.def to the project now.
(This is an annoying little file with only two lines.  An alternative
approach is to forget about the .def file, and add the option
"/export:initspam" somewhere to the Link settings, by manually editing the
"Project -> Properties -> Linker -> Command Line -> Additional Options" 
box).

You are now all set to build your extension, unless it requires other
external libraries, include files, etc.  See Python's Extending and
Embedding manual for instructions on how to write an extension.


CREATING A BRAND NEW PROJECT
----------------------------
Use the
    File -> New -> Project...
dialog to create a new Project Workspace.  Select "Visual C++ Projects/Win32/
Win32 Project", enter the name ("spam"), and make sure the "Location" is 
set to parent of the spam directory you have created (which should be a direct 
subdirectory of the Python build tree, a sibling of Include and PC).  
In "Application Settings", select "DLL", and "Empty Project".  Click OK.

You should now create the file spam.def as instructed in the previous
section. Add the source files (including the .def file) to the project, 
using "Project", "Add Existing Item".

Now open the
    Project -> spam properties...
dialog.  (Impressive, isn't it? :-) You only need to change a few
settings.  Make sure "All Configurations" is selected from the "Settings
for:" dropdown list.  Select the "C/C++" tab.  Choose the "General"
category in the popup menu at the top.  Type the following text in the
entry box labeled "Addditional Include Directories:"

    ..\Include,..\PC

Then, choose the "General" category in the "Linker" tab, and enter
    ..\PCbuild
in the "Additional library Directories" box.

Now you need to add some mode-specific settings (select "Accept"
when asked to confirm your changes):

Select "Release" in the "Configuration" dropdown list.  Click the
"Link" tab, choose the "Input" Category, and append "python24.lib" to the
list in the "Additional Dependencies" box.

Select "Debug" in the "Settings for:" dropdown list, and append
"python24_d.lib" to the list in the Additional Dependencies" box.  Then
click on the C/C++ tab, select "Code Generation", and select 
"Multi-threaded Debug DLL" from the "Runtime library" dropdown list.

Select "Release" again from the "Settings for:" dropdown list.
Select "Multi-threaded DLL" from the "Use run-time library:" dropdown list.

That's all <wink>.
