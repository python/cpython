
.. _using-on-mac:

*********************
Using Python on a Mac
*********************

:Author: Bob Savage <bobsavage@mac.com>


Python on a Mac running macOS is in principle very similar to Python on
any other Unix platform, but there are a number of additional features such as
the integrated development environment (IDE) and the Package Manager that are
worth pointing out.


.. _getting-osx:
.. _getting-and-installing-macpython:

Getting and Installing Python
=============================

macOS used to come with Python 2.7 pre-installed between versions
10.8 and `12.3 <https://developer.apple.com/documentation/macos-release-notes/macos-12_3-release-notes#Python>`_.
You are invited to install the most recent version of Python 3 from the `Python
website <https://www.python.org/downloads/macos/>`__.
A current "universal2 binary" build of Python, which runs natively on the Mac's
new Apple Silicon and legacy Intel processors, is available there.

What you get after installing is a number of things:

* A |python_version_literal| folder in your :file:`Applications` folder. In here
  you find IDLE, the development environment that is a standard part of official
  Python distributions; and :program:`Python Launcher`, which handles double-clicking Python
  scripts from the Finder.

* A framework :file:`/Library/Frameworks/Python.framework`, which includes the
  Python executable and libraries. The installer adds this location to your shell
  path. To uninstall Python, you can remove these three things. A
  symlink to the Python executable is placed in :file:`/usr/local/bin/`.

.. note::

   On macOS 10.8-12.3, the Apple-provided build of Python is installed in
   :file:`/System/Library/Frameworks/Python.framework` and :file:`/usr/bin/python`,
   respectively. You should never modify or delete these, as they are
   Apple-controlled and are used by Apple- or third-party software.  Remember that
   if you choose to install a newer Python version from python.org, you will have
   two different but functional Python installations on your computer, so it will
   be important that your paths and usages are consistent with what you want to do.

IDLE includes a Help menu that allows you to access Python documentation. If you
are completely new to Python you should start reading the tutorial introduction
in that document.

If you are familiar with Python on other Unix platforms you should read the
section on running Python scripts from the Unix shell.


How to run a Python script
--------------------------

Your best way to get started with Python on macOS is through the IDLE
integrated development environment; see section :ref:`ide` and use the Help menu
when the IDE is running.

If you want to run Python scripts from the Terminal window command line or from
the Finder you first need an editor to create your script. macOS comes with a
number of standard Unix command line editors, :program:`vim`
:program:`nano` among them. If you want a more Mac-like editor,
:program:`BBEdit` from Bare Bones Software (see
https://www.barebones.com/products/bbedit/index.html) are good choices, as is
:program:`TextMate` (see https://macromates.com). Other editors include
:program:`MacVim` (https://macvim.org) and :program:`Aquamacs`
(https://aquamacs.org).

To run your script from the Terminal window you must make sure that
:file:`/usr/local/bin` is in your shell search path.

To run your script from the Finder you have two options:

* Drag it to :program:`Python Launcher`.

* Select :program:`Python Launcher` as the default application to open your
  script (or any ``.py`` script) through the finder Info window and double-click it.
  :program:`Python Launcher` has various preferences to control how your script is
  launched. Option-dragging allows you to change these for one invocation, or use
  its Preferences menu to change things globally.


.. _osx-gui-scripts:

Running scripts with a GUI
--------------------------

With older versions of Python, there is one macOS quirk that you need to be
aware of: programs that talk to the Aqua window manager (in other words,
anything that has a GUI) need to be run in a special way. Use :program:`pythonw`
instead of :program:`python` to start such scripts.

With Python 3.9, you can use either :program:`python` or :program:`pythonw`.


Configuration
-------------

Python on macOS honors all standard Unix environment variables such as
:envvar:`PYTHONPATH`, but setting these variables for programs started from the
Finder is non-standard as the Finder does not read your :file:`.profile` or
:file:`.cshrc` at startup. You need to create a file
:file:`~/.MacOSX/environment.plist`. See Apple's
`Technical Q&A QA1067 <https://developer.apple.com/library/archive/qa/qa1067/_index.html>`__
for details.

For more information on installation Python packages, see section
:ref:`mac-package-manager`.


.. _ide:

The IDE
=======

Python ships with the standard IDLE development environment. A good
introduction to using IDLE can be found at
https://www.hashcollision.org/hkn/python/idle_intro/index.html.


.. _mac-package-manager:

Installing Additional Python Packages
=====================================

This section has moved to the `Python Packaging User Guide`_.

.. _Python Packaging User Guide: https://packaging.python.org/en/latest/tutorials/installing-packages/


.. _gui-programming-on-the-mac:

GUI Programming
===============

There are several options for building GUI applications on the Mac with Python.

*PyObjC* is a Python binding to Apple's Objective-C/Cocoa framework, which is
the foundation of most modern Mac development. Information on PyObjC is
available from https://pypi.org/project/pyobjc/.

The standard Python GUI toolkit is :mod:`tkinter`, based on the cross-platform
Tk toolkit (https://www.tcl.tk). An Aqua-native version of Tk is bundled with OS
X by Apple, and the latest version can be downloaded and installed from
https://www.activestate.com; it can also be built from source.

A number of alternative macOS GUI toolkits are available:

* `PySide <https://www.qt.io/qt-for-python>`__: Official Python bindings to the
  `Qt GUI toolkit <https://qt.io>`__.

* `PyQt <https://riverbankcomputing.com/software/pyqt/intro>`__: Alternative
  Python bindings to Qt.

* `Kivy <https://kivy.org>`__: A cross-platform GUI toolkit that supports
  desktop and mobile platforms.

* `Toga <https://toga.readthedocs.io>`__: Part of the `BeeWare Project
  <https://beeware.org>`__; supports desktop, mobile, web and console apps.

* `wxPython <https://www.wxpython.org>`__: A cross-platform toolkit that
  supports desktop operating systems.

.. _distributing-python-applications-on-the-mac:

Distributing Python Applications
================================

A range of tools exist for converting your Python code into a standalone
distributable application:

* `py2app <https://pypi.org/project/py2app/>`__: Supports creating macOS ``.app``
  bundles from a Python project.

* `Briefcase <https://briefcase.readthedocs.io>`__: Part of the `BeeWare Project
  <https://beeware.org>`__; a cross-platform packaging tool that supports
  creation of ``.app`` bundles on macOS, as well as managing signing and
  notarization.

* `PyInstaller <https://pyinstaller.org/>`__: A cross-platform packaging tool that creates
  a single file or folder as a distributable artifact.

Other Resources
===============

The Pythonmac-SIG mailing list is an excellent support resource for Python users
and developers on the Mac:

https://www.python.org/community/sigs/current/pythonmac-sig/

Another useful resource is the MacPython wiki:

https://wiki.python.org/moin/MacPython
