
.. _toolbox:

**********************
Mac OS Toolbox Modules
**********************

There are a set of modules that provide interfaces to various Mac OS toolboxes.
If applicable the module will define a number of Python objects for the various
structures declared by the toolbox, and operations will be implemented as
methods of the object.  Other operations will be implemented as functions in the
module.  Not all operations possible in C will also be possible in Python
(callbacks are often a problem), and parameters will occasionally be different
in Python (input and output buffers, especially).  All methods and functions
have a :attr:`__doc__` string describing their arguments and return values, and
for additional description you are referred to `Inside Macintosh
<http://developer.apple.com/legacy/mac/library/#documentation/macos8/mac8.html>`_ or similar works.

These modules all live in a package called :mod:`Carbon`. Despite that name they
are not all part of the Carbon framework: CF is really in the CoreFoundation
framework and Qt is in the QuickTime framework. The normal use pattern is ::

   from Carbon import AE

.. note::

   The Carbon modules have been removed in Python 3.


:mod:`Carbon.AE` --- Apple Events
=================================

.. module:: Carbon.AE
   :platform: Mac
   :synopsis: Interface to the Apple Events toolbox.
   :deprecated:



:mod:`Carbon.AH` --- Apple Help
===============================

.. module:: Carbon.AH
   :platform: Mac
   :synopsis: Interface to the Apple Help manager.
   :deprecated:



:mod:`Carbon.App` --- Appearance Manager
========================================

.. module:: Carbon.App
   :platform: Mac
   :synopsis: Interface to the Appearance Manager.
   :deprecated:

:mod:`Carbon.Appearance` --- Appearance Manager constants
=========================================================

.. module:: Carbon.Appearance
   :platform: Mac
   :synopsis: Constant definitions for the interface to the Appearance Manager.
   :deprecated:



:mod:`Carbon.CF` --- Core Foundation
====================================

.. module:: Carbon.CF
   :platform: Mac
   :synopsis: Interface to the Core Foundation.
   :deprecated:


The ``CFBase``, ``CFArray``, ``CFData``, ``CFDictionary``, ``CFString`` and
``CFURL`` objects are supported, some only partially.


:mod:`Carbon.CG` --- Core Graphics
==================================

.. module:: Carbon.CG
   :platform: Mac
   :synopsis: Interface to Core Graphics.
   :deprecated:



:mod:`Carbon.CarbonEvt` --- Carbon Event Manager
================================================

.. module:: Carbon.CarbonEvt
   :platform: Mac
   :synopsis: Interface to the Carbon Event Manager.
   :deprecated:

:mod:`Carbon.CarbonEvents` --- Carbon Event Manager constants
=============================================================

.. module:: Carbon.CarbonEvents
   :platform: Mac
   :synopsis: Constants for the interface to the Carbon Event Manager.
   :deprecated:



:mod:`Carbon.Cm` --- Component Manager
======================================

.. module:: Carbon.Cm
   :platform: Mac
   :synopsis: Interface to the Component Manager.
   :deprecated:

:mod:`Carbon.Components` --- Component Manager constants
========================================================

.. module:: Carbon.Components
   :platform: Mac
   :synopsis: Constants for the interface to the Component Manager.
   :deprecated:


:mod:`Carbon.ControlAccessor` --- Control Manager accssors
===========================================================

.. module:: Carbon.ControlAccessor
   :platform: Mac
   :synopsis: Accessor functions for the interface to the Control Manager.
   :deprecated:

:mod:`Carbon.Controls` --- Control Manager constants
====================================================

.. module:: Carbon.Controls
   :platform: Mac
   :synopsis: Constants for the interface to the Control Manager.
   :deprecated:

:mod:`Carbon.CoreFounation` --- CoreFounation constants
=======================================================

.. module:: Carbon.CoreFounation
   :platform: Mac
   :synopsis: Constants for the interface to CoreFoundation.
   :deprecated:

:mod:`Carbon.CoreGraphics` --- CoreGraphics constants
=======================================================

.. module:: Carbon.CoreGraphics
   :platform: Mac
   :synopsis: Constants for the interface to CoreGraphics.
   :deprecated:

:mod:`Carbon.Ctl` --- Control Manager
=====================================

.. module:: Carbon.Ctl
   :platform: Mac
   :synopsis: Interface to the Control Manager.
   :deprecated:

:mod:`Carbon.Dialogs` --- Dialog Manager constants
==================================================

.. module:: Carbon.Dialogs
   :platform: Mac
   :synopsis: Constants for the interface to the Dialog Manager.
   :deprecated:

:mod:`Carbon.Dlg` --- Dialog Manager
====================================

.. module:: Carbon.Dlg
   :platform: Mac
   :synopsis: Interface to the Dialog Manager.
   :deprecated:

:mod:`Carbon.Drag` --- Drag and Drop Manager
=============================================

.. module:: Carbon.Drag
   :platform: Mac
   :synopsis: Interface to the Drag and Drop Manager.
   :deprecated:

:mod:`Carbon.Dragconst` --- Drag and Drop Manager constants
===========================================================

.. module:: Carbon.Dragconst
   :platform: Mac
   :synopsis: Constants for the interface to the Drag and Drop Manager.
   :deprecated:

:mod:`Carbon.Events` --- Event Manager constants
================================================

.. module:: Carbon.Events
   :platform: Mac
   :synopsis: Constants for the interface to the classic Event Manager.
   :deprecated:

:mod:`Carbon.Evt` --- Event Manager
===================================

.. module:: Carbon.Evt
   :platform: Mac
   :synopsis: Interface to the classic Event Manager.
   :deprecated:

:mod:`Carbon.File` --- File Manager
===================================

.. module:: Carbon.File
   :platform: Mac
   :synopsis: Interface to the File Manager.
   :deprecated:

:mod:`Carbon.Files` --- File Manager constants
==============================================

.. module:: Carbon.Files
   :platform: Mac
   :synopsis: Constants for the interface to the File Manager.
   :deprecated:


:mod:`Carbon.Fm` --- Font Manager
=================================

.. module:: Carbon.Fm
   :platform: Mac
   :synopsis: Interface to the Font Manager.
   :deprecated:



:mod:`Carbon.Folder` --- Folder Manager
=======================================

.. module:: Carbon.Folder
   :platform: Mac
   :synopsis: Interface to the Folder Manager.
   :deprecated:

:mod:`Carbon.Folders` --- Folder Manager constants
==================================================

.. module:: Carbon.Folders
   :platform: Mac
   :synopsis: Constants for the interface to the Folder Manager.
   :deprecated:


:mod:`Carbon.Fonts` --- Font Manager constants
==================================================

.. module:: Carbon.Fonts
   :platform: Mac
   :synopsis: Constants for the interface to the Font Manager.
   :deprecated:



:mod:`Carbon.Help` --- Help Manager
===================================

.. module:: Carbon.Help
   :platform: Mac
   :synopsis: Interface to the Carbon Help Manager.
   :deprecated:

:mod:`Carbon.IBCarbon` --- Carbon InterfaceBuilder
==================================================

.. module:: Carbon.IBCarbon
   :platform: Mac
   :synopsis: Interface to the Carbon InterfaceBuilder support libraries.
   :deprecated:

:mod:`Carbon.IBCarbonRuntime` --- Carbon InterfaceBuilder constants
===================================================================

.. module:: Carbon.IBCarbonRuntime
   :platform: Mac
   :synopsis: Constants for the interface to the Carbon InterfaceBuilder support libraries.
   :deprecated:

:mod:`Carbon.Icn` --- Carbon Icon Manager
=========================================

.. module:: Carbon.Icns
   :platform: Mac
   :synopsis: Interface to the Carbon Icon Manager
   :deprecated:

:mod:`Carbon.Icons` --- Carbon Icon Manager constants
=====================================================

.. module:: Carbon.Icons
   :platform: Mac
   :synopsis: Constants for the interface to the Carbon Icon Manager
   :deprecated:

:mod:`Carbon.Launch` --- Carbon Launch Services
===============================================

.. module:: Carbon.Launch
   :platform: Mac
   :synopsis: Interface to the Carbon Launch Services.
   :deprecated:

:mod:`Carbon.LaunchServices` --- Carbon Launch Services constants
=================================================================

.. module:: Carbon.LaunchServices
   :platform: Mac
   :synopsis: Constants for the interface to the Carbon Launch Services.
   :deprecated:


:mod:`Carbon.List` --- List Manager
===================================

.. module:: Carbon.List
   :platform: Mac
   :synopsis: Interface to the List Manager.
   :deprecated:



:mod:`Carbon.Lists` --- List Manager constants
==============================================

.. module:: Carbon.Lists
   :platform: Mac
   :synopsis: Constants for the interface to the List Manager.
   :deprecated:

:mod:`Carbon.MacHelp` --- Help Manager constants
================================================

.. module:: Carbon.MacHelp
   :platform: Mac
   :synopsis: Constants for the interface to the Carbon Help Manager.
   :deprecated:

:mod:`Carbon.MediaDescr` --- Parsers and generators for Quicktime Media descriptors
===================================================================================

.. module:: Carbon.MediaDescr
   :platform: Mac
   :synopsis: Parsers and generators for Quicktime Media descriptors
   :deprecated:


:mod:`Carbon.Menu` --- Menu Manager
===================================

.. module:: Carbon.Menu
   :platform: Mac
   :synopsis: Interface to the Menu Manager.
   :deprecated:

:mod:`Carbon.Menus` --- Menu Manager constants
==============================================

.. module:: Carbon.Menus
   :platform: Mac
   :synopsis: Constants for the interface to the Menu Manager.
   :deprecated:


:mod:`Carbon.Mlte` --- MultiLingual Text Editor
===============================================

.. module:: Carbon.Mlte
   :platform: Mac
   :synopsis: Interface to the MultiLingual Text Editor.
   :deprecated:

:mod:`Carbon.OSA` --- Carbon OSA Interface
==========================================

.. module:: Carbon.OSA
   :platform: Mac
   :synopsis: Interface to the Carbon OSA Library.
   :deprecated:

:mod:`Carbon.OSAconst` --- Carbon OSA Interface constants
=========================================================

.. module:: Carbon.OSAconst
   :platform: Mac
   :synopsis: Constants for the interface to the Carbon OSA Library.
   :deprecated:

:mod:`Carbon.QDOffscreen` --- QuickDraw Offscreen constants
===========================================================

.. module:: Carbon.QDOffscreen
   :platform: Mac
   :synopsis: Constants for the interface to the QuickDraw Offscreen APIs.
   :deprecated:


:mod:`Carbon.Qd` --- QuickDraw
==============================

.. module:: Carbon.Qd
   :platform: Mac
   :synopsis: Interface to the QuickDraw toolbox.
   :deprecated:



:mod:`Carbon.Qdoffs` --- QuickDraw Offscreen
============================================

.. module:: Carbon.Qdoffs
   :platform: Mac
   :synopsis: Interface to the QuickDraw Offscreen APIs.
   :deprecated:



:mod:`Carbon.Qt` --- QuickTime
==============================

.. module:: Carbon.Qt
   :platform: Mac
   :synopsis: Interface to the QuickTime toolbox.
   :deprecated:

:mod:`Carbon.QuickDraw` --- QuickDraw constants
===============================================

.. module:: Carbon.QuickDraw
   :platform: Mac
   :synopsis: Constants for the interface to the QuickDraw toolbox.
   :deprecated:

:mod:`Carbon.QuickTime` --- QuickTime constants
===============================================

.. module:: Carbon.QuickTime
   :platform: Mac
   :synopsis: Constants for the interface to the QuickTime toolbox.
   :deprecated:


:mod:`Carbon.Res` --- Resource Manager and Handles
==================================================

.. module:: Carbon.Res
   :platform: Mac
   :synopsis: Interface to the Resource Manager and Handles.
   :deprecated:

:mod:`Carbon.Resources` --- Resource Manager and Handles constants
==================================================================

.. module:: Carbon.Resources
   :platform: Mac
   :synopsis: Constants for the interface to the Resource Manager and Handles.
   :deprecated:


:mod:`Carbon.Scrap` --- Scrap Manager
=====================================

.. module:: Carbon.Scrap
   :platform: Mac
   :synopsis: The Scrap Manager provides basic services for implementing cut & paste and
              clipboard operations.
   :deprecated:


This module is only fully available on Mac OS 9 and earlier under classic PPC
MacPython.  Very limited functionality is available under Carbon MacPython.

.. index:: single: Scrap Manager

The Scrap Manager supports the simplest form of cut & paste operations on the
Macintosh.  It can be use for both inter- and intra-application clipboard
operations.

The :mod:`Scrap` module provides low-level access to the functions of the Scrap
Manager.  It contains the following functions:


.. function:: InfoScrap()

   Return current information about the scrap.  The information is encoded as a
   tuple containing the fields ``(size, handle, count, state, path)``.

   +----------+---------------------------------------------+
   | Field    | Meaning                                     |
   +==========+=============================================+
   | *size*   | Size of the scrap in bytes.                 |
   +----------+---------------------------------------------+
   | *handle* | Resource object representing the scrap.     |
   +----------+---------------------------------------------+
   | *count*  | Serial number of the scrap contents.        |
   +----------+---------------------------------------------+
   | *state*  | Integer; positive if in memory, ``0`` if on |
   |          | disk, negative if uninitialized.            |
   +----------+---------------------------------------------+
   | *path*   | Filename of the scrap when stored on disk.  |
   +----------+---------------------------------------------+


.. seealso::

   `Scrap Manager <http://developer.apple.com/legacy/mac/library/documentation/mac/MoreToolbox/MoreToolbox-109.html>`_
      Apple's documentation for the Scrap Manager gives a lot of useful information
      about using the Scrap Manager in applications.



:mod:`Carbon.Snd` --- Sound Manager
===================================

.. module:: Carbon.Snd
   :platform: Mac
   :synopsis: Interface to the Sound Manager.
   :deprecated:

:mod:`Carbon.Sound` --- Sound Manager constants
===============================================

.. module:: Carbon.Sound
   :platform: Mac
   :synopsis: Constants for the interface to the Sound Manager.
   :deprecated:


:mod:`Carbon.TE` --- TextEdit
=============================

.. module:: Carbon.TE
   :platform: Mac
   :synopsis: Interface to TextEdit.
   :deprecated:

:mod:`Carbon.TextEdit` --- TextEdit constants
=============================================

.. module:: Carbon.TextEdit
   :platform: Mac
   :synopsis: Constants for the interface to TextEdit.
   :deprecated:



:mod:`Carbon.Win` --- Window Manager
====================================

.. module:: Carbon.Win
   :platform: Mac
   :synopsis: Interface to the Window Manager.
   :deprecated:

:mod:`Carbon.Windows` --- Window Manager constants
==================================================

.. module:: Carbon.Windows
   :platform: Mac
   :synopsis: Constants for the interface to the Window Manager.
   :deprecated:
