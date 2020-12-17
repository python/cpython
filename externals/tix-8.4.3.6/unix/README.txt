$Id: README.txt,v 1.4 2004/10/02 01:25:52 hobbs Exp $

        ---   Build Tix binaries for the Unix platforms   ---

Before you start
================

    The Tix home page is at

    http://tix.sourceforge.net/

    This site also has more information in case you get stuck, such as
    who to contact for questions.

Required Tcl/Tk versions
========================

    You need Tcl/Tk 8.2 or later. Prior versions are no longer
    supported. Tcl 8.4 is the recommended version.

Install the Tcl/Tk/Tix sources
==============================

    Tix is a standard Tcl Extension Architecture (TEA) based extension.
    You must have Tcl and Tk on your system prior to building Tix.

Run the Tix configure script
============================

    To see the options for the Tix configure script:

        cd /src/tix/
        ./configure --help

    Tix builds as a stubs-enabled shared extension to Tcl/Tk by default.

    You may want to set the --prefix and --exec-prefix options if
    you want to install Tix in a directory other than /usr/local.

    If you have several versions of Tcl/Tk sources installed in your
    source directory, you can use the --with-tcl and --with-tk options
    to choose which version to build Tix with.

Build Tix
=========

    After you run the configure script, you'd get a Makefile for
    building Tix. Then, just invoke the "make" command to build the
    Tix binaries.

        cd /src/tix/
        make

Test Tix
========

    You may want to run the Tix regression test suite to see if things
    work as expected:

        cd /src/tix/
        make tests

    You can also run the Tix widget demos:

        cd /src/tix/
        make rundemos

Install Tix
===========

    When you're satisfied with the Tix build, you can install it by
    invoking:

        cd /src/tix/
        make install

    This by default will install the Tix files into the following
    places:

        /usr/local/lib/tix8.4/libtix8.4.so    shared library
        /usr/local/lib/tix8.4/*.tcl           script library
        /usr/local/man/mann/*.n               man pages

    The installation directory may be set using the --prefix and
    --exec-prefix options  to the configure script.

    You may also need to install Tcl and Tk if you haven't already
    done so.

Test the installation
=====================

    To make sure everything works after "make install", do this:

        env TIX_LIBRARY=/set/correct/path wish /src/tix/tests/all.tcl

    This will ensure Tcl/Tk and Tix can work without any environment
    settings.
