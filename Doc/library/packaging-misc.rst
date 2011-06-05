.. temporary file for modules that don't need a dedicated file yet

:mod:`packaging.errors` --- Packaging exceptions
================================================

.. module:: packaging.errors
   :synopsis: Packaging exceptions.


Provides exceptions used by the Packaging modules.  Note that Packaging modules
may raise standard exceptions; in particular, SystemExit is usually raised for
errors that are obviously the end-user's fault (e.g. bad command-line arguments).

This module is safe to use in ``from ... import *`` mode; it only exports
symbols whose names start with ``Packaging`` and end with ``Error``.


:mod:`packaging.manifest` --- The Manifest class
================================================

.. module:: packaging.manifest
   :synopsis: The Manifest class, used for poking about the file system and
              building lists of files.


This module provides the :class:`Manifest` class, used for poking about the
filesystem and building lists of files.
