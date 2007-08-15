
.. _undoc:

********************
Undocumented Modules
********************

Here's a quick listing of modules that are currently undocumented, but that
should be documented.  Feel free to contribute documentation for them!  (Send
via email to docs@python.org.)

The idea and original contents for this chapter were taken from a posting by
Fredrik Lundh; the specific contents of this chapter have been substantially
revised.


Miscellaneous useful utilities
==============================

Some of these are very old and/or not very robust; marked with "hmm."

:mod:`bdb`
   --- A generic Python debugger base class (used by pdb).

:mod:`ihooks`
   --- Import hook support (for :mod:`rexec`; may become obsolete).


Platform specific modules
=========================

These modules are used to implement the :mod:`os.path` module, and are not
documented beyond this mention.  There's little need to document these.

:mod:`ntpath`
   --- Implementation of :mod:`os.path` on Win32, Win64, WinCE, and OS/2 platforms.

:mod:`posixpath`
   --- Implementation of :mod:`os.path` on POSIX.


Multimedia
==========

:mod:`linuxaudiodev`
   --- Play audio data on the Linux audio device.  Replaced in Python 2.3 by the
   :mod:`ossaudiodev` module.

:mod:`sunaudio`
   --- Interpret Sun audio headers (may become obsolete or a tool/demo).


.. _undoc-mac-modules:

Undocumented Mac OS modules
===========================


:mod:`applesingle` --- AppleSingle decoder
------------------------------------------

.. module:: applesingle
   :platform: Mac
   :synopsis: Rudimentary decoder for AppleSingle format files.



:mod:`buildtools` --- Helper module for BuildApplet and Friends
---------------------------------------------------------------

.. module:: buildtools
   :platform: Mac
   :synopsis: Helper module for BuildApplet, BuildApplication and macfreeze.


.. deprecated:: 2.4


:mod:`icopen` --- Internet Config replacement for :meth:`open`
--------------------------------------------------------------

.. module:: icopen
   :platform: Mac
   :synopsis: Internet Config replacement for open().


Importing :mod:`icopen` will replace the builtin :meth:`open` with a version
that uses Internet Config to set file type and creator for new files.


:mod:`macerrors` --- Mac OS Errors
----------------------------------

.. module:: macerrors
   :platform: Mac
   :synopsis: Constant definitions for many Mac OS error codes.


:mod:`macerrors` contains constant definitions for many Mac OS error codes.


:mod:`macresource` --- Locate script resources
----------------------------------------------

.. module:: macresource
   :platform: Mac
   :synopsis: Locate script resources.


:mod:`macresource` helps scripts finding their resources, such as dialogs and
menus, without requiring special case code for when the script is run under
MacPython, as a MacPython applet or under OSX Python.


:mod:`Nav` --- NavServices calls
--------------------------------

.. module:: Nav
   :platform: Mac
   :synopsis: Interface to Navigation Services.


A low-level interface to Navigation Services.


:mod:`PixMapWrapper` --- Wrapper for PixMap objects
---------------------------------------------------

.. module:: PixMapWrapper
   :platform: Mac
   :synopsis: Wrapper for PixMap objects.


:mod:`PixMapWrapper` wraps a PixMap object with a Python object that allows
access to the fields by name. It also has methods to convert to and from
:mod:`PIL` images.


:mod:`videoreader` --- Read QuickTime movies
--------------------------------------------

.. module:: videoreader
   :platform: Mac
   :synopsis: Read QuickTime movies frame by frame for further processing.


:mod:`videoreader` reads and decodes QuickTime movies and passes a stream of
images to your program. It also provides some support for audio tracks.


:mod:`W` --- Widgets built on :mod:`FrameWork`
----------------------------------------------

.. module:: W
   :platform: Mac
   :synopsis: Widgets for the Mac, built on top of FrameWork.


The :mod:`W` widgets are used extensively in the :program:`IDE`.


.. _obsolete-modules:

Obsolete
========

These modules are not normally available for import; additional work must be
done to make them available.

These extension modules written in C are not built by default. Under Unix, these
must be enabled by uncommenting the appropriate lines in :file:`Modules/Setup`
in the build tree and either rebuilding Python if the modules are statically
linked, or building and installing the shared object if using dynamically-loaded
extensions.

.. % %% lib-old is empty as of Python 2.5
.. % Those which are written in Python will be installed into the directory
.. % \file{lib-old/} installed as part of the standard library.  To use
.. % these, the directory must be added to \code{sys.path}, possibly using
.. % \envvar{PYTHONPATH}.

.. % XXX need Windows instructions!


   --- This section should be empty for Python 3.0.

