RCS $Id: README.txt,v 1.7 2004/10/02 01:25:52 hobbs Exp $

        ---   Build Tix binaries for the Win32 platform   ---

Before you start
================

    If you are not familiar with Windows or do not have a working
    win32 compiler, you can download a Tix windows binary from

    http://tix.sourceforge.net/

    This site also has more information in case you get stuck, such as
    who to contact for questions.

Required Tcl/Tk versions
========================

    You need Tcl/Tk 8.2 or later. Prior versions are no longer
    supported. Tcl 8.4 is the recommended version.

Supported Compilers
===================

  * MSVC++ 6:                    use makefile.vc
  * cygwin + mingw/gcc | MSVC:   use ../configure

    If you don't want to pay for the VC++ compiler, you can get Cygwin
    from http://www.cygwin.com/.  mingw/gcc is the recommended Windows
    gcc variant.

    No other compilers are currently supported.
    Please post your patches to	http://tix.sourceforge.net.
   
Customizing your build
======================

    The recommended method of customizing your build is to create a file
    called "Makefile" in this directory. Set the MAKE variables that you
    want to modify. Then, include the makefile.vc.

    You can look at the top of the makefile for the variables that you can
    modify.

    For example, if you use VC++, and you want to change the version
    of Tcl/Tk to build with, create a Makefile like this:

    ------------------------------------------------------------------
    # My own makefile ...
    TCL_MAJOR 	= 8
    TCL_MINOR 	= 4
    TCL_PATCH 	= 7
    !include "makefile.vc"
    ------------------------------------------------------------------

    The advantage of this method is you can reuse your customization
    Makefile across different Tix source releases without doing the
    same modifications again and again.

Building the binaries
=====================

    + First, you need to download the Tcl/Tk sources and install them to
      along with Tix inside the same directory. You can download Tcl/Tk from

      http://www.tcl.tk/

    + If you use VC++: build both Tcl and Tk using the win/makefile.vc
      files that come with Tcl and Tk.

    + If you use Cygwin, download the Tcl binary distribution of the
      same version as the Tcl/Tk sources from
      http://www.tcl.tk/ and install it on your PC.

    + Create the customization Makefile for Tix as mentioned above.

    + Execute your favorite MAKE program in this directory.
      E.g., if you use VC++:

        cd win
        nmake

      If you use Cygwin:

        cd win
        make

    This should produce various .DLL and .EXE files in the Release or
    Debug subdirectories.

Testing your build
==================

    Run the following command in this directory to run the Tix
    regression test suite.

        nmake test              -- with VC++, or
        make test               -- with Cygwin

    Run the following command in this directory to run the Tix
    widget demos.

        nmake rundemos          -- with VC++, or
        make rundemos           -- with Cygwin


Installing Tix
==============

    The makefiles in this directory has a crude method of installing
    Tix on your local machine. E.g.,

        nmake install  

    The default installation directory is C:\Tcl. You can customize this
    location by setting the INSTALL_DIR variable in your customization
    Makefile.

    Nevertheless, if you're planning a wide distribution of Tix across
    many PC's, you probably need to create an installer program or use
    more advanced administrator tools.

Using Tix in your Tcl scripts
=============================

    Once Tix is installed properly on your machine, simple execute the
    "package require Tix" command in your Tcl scripts to access the
    Tix features.

    See the file ../demos/widget for examples.
