#! /usr/bin/env python
"""Tkinter is the Python interface to the Tk GUI toolkit.  Tk offers
native look and feel on most major platforms, including Unix, Windows,
and Macintosh.  The Tkinter-2.0 RPM contains the Python C extension
module for Python 2.0.  The Python source files are distributed with
the main Python distribution."""

from distutils.core import setup, Extension

setup(name="Tkinter-2.0",
      version="8.0",
      description="Python interface to Tk GUI toolkit",
      author="Python development team",
      author_email="pythoneers@beopen.com",
      url="http://www.pythonlabs.com/products/python2.0/",
      licence="Modified CNRI Open Source License",
      
      ext_modules=[Extension("_tkinter",
                            ["src/_tkinter.c", "src/tkappinit.c"],
                            define_macros=[('WITH_APPINIT', None)],
                            library_dirs=["/usr/X11R6/lib"],
                            libraries=["tk8.0", "tcl8.0", "X11"],
                            )],

      long_description = __doc__
      )
                                         
