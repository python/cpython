
:mod:`tabnanny` --- Detection of ambiguous indentation
======================================================

.. module:: tabnanny
   :synopsis: Tool for detecting white space related problems in Python source files in a
              directory tree.
.. moduleauthor:: Tim Peters <tim_one@users.sourceforge.net>
.. sectionauthor:: Peter Funk <pf@artcom-gmbh.de>


.. % rudimentary documentation based on module comments, by Peter Funk
.. % <pf@artcom-gmbh.de>

For the time being this module is intended to be called as a script. However it
is possible to import it into an IDE and use the function :func:`check`
described below.

.. warning::

   The API provided by this module is likely to change  in future releases; such
   changes may not be backward compatible.


.. function:: check(file_or_dir)

   If *file_or_dir* is a directory and not a symbolic link, then recursively
   descend the directory tree named by *file_or_dir*, checking all :file:`.py`
   files along the way.  If *file_or_dir* is an ordinary Python source file, it is
   checked for whitespace related problems.  The diagnostic messages are written to
   standard output using the print statement.


.. data:: verbose

   Flag indicating whether to print verbose messages. This is incremented by the
   ``-v`` option if called as a script.


.. data:: filename_only

   Flag indicating whether to print only the filenames of files containing
   whitespace related problems.  This is set to true by the ``-q`` option if called
   as a script.


.. exception:: NannyNag

   Raised by :func:`tokeneater` if detecting an ambiguous indent. Captured and
   handled in :func:`check`.


.. function:: tokeneater(type, token, start, end, line)

   This function is used by :func:`check` as a callback parameter to the function
   :func:`tokenize.tokenize`.

.. % XXX FIXME: Document \function{errprint},
.. % \function{format_witnesses} \class{Whitespace}
.. % check_equal, indents
.. % \function{reset_globals}


.. seealso::

   Module :mod:`tokenize`
      Lexical scanner for Python source code.

   .. % XXX may be add a reference to IDLE?

