
I. Building the Tcl thread extension for Windows
================================================

Thread extension supports two build options:


o. MinGW builds:
----------------

The extension can be compiled under Windows using the
MinGW (http://www.mingw.org) environment. You can also
download the ready-to-go copy of the MinGW from the
same place you've downloaded this extension.

You should compile the Tcl core with MinGW first. After
that, you can compile the extension by running the
configure/make from this directory. You can also use the
CONFIG script to do this. You might want to edit the
script to match your environment and then just do:

    sh CONFIG

This should go smoothly, once you got Tcl core compiled ok.


o. Microsoft MSVC++ build:
--------------------------

Files in this directory may be useful if you have not set up
your TEA (i.e., MinGW) environment and you're using the MSVC++
from Micro$oft.

To build the extension invoke the following command:

    nmake -f makefile.vc INSTALLDIR=<path-to-installed-tcl>

INSTALLDIR is the path of the Tcl distribution where
tcl.h and other needed Tcl files are installed.
To build against a Tcl source build instead,

    nmake -f makefile.vc TCLDIR=<path-to-tcl-sources>

Please look into the makefile.vc file for more options etc.

Alternatively, you can open the extension workspace and project files
(thread_win.dsw and thread_win.dsp) from within the MSVC++ and press
the F7 key to build the extension under the control of the MSVC IDE.
NOTE: it is likely that the .dsw and .dsp files are out of date. At
least Visual Studio 2017 was not able to open those files.

II. Building optional support libraries
=======================================

As of 2.6 release, this extension supports persistent shared
variables. To use this functionality, you might need to download
and compile some other supporting libraries. Currently, there is
a simple implementation of shared variable persistency built atop
of popular GNU Gdbm package. You can obtain the latest version of
the Gdbm from: http://www.gnu.org/software/gdbm/gdbm.html.

For the impatient, there are Windows ports of GNU Gdbm found on
various places on the Internet. The easiest way to start is to go
to the GnuWin32 project: http://sourceforge.net/projects/gnuwin32
and fetch yourself a compiled GNU Gdbm DLL.

-EOF-
