
:mod:`ColorPicker` --- Color selection dialog
=============================================

.. module:: ColorPicker
   :platform: Mac
   :synopsis: Interface to the standard color selection dialog.
   :deprecated:
.. moduleauthor:: Just van Rossum <just@letterror.com>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


The :mod:`ColorPicker` module provides access to the standard color picker
dialog.

.. note::

   This module has been removed in Python 3.x.


.. function:: GetColor(prompt, rgb)

   Show a standard color selection dialog and allow the user to select a color.
   The user is given instruction by the *prompt* string, and the default color is
   set to *rgb*.  *rgb* must be a tuple giving the red, green, and blue components
   of the color. :func:`GetColor` returns a tuple giving the user's selected color
   and a flag indicating whether they accepted the selection of cancelled.

