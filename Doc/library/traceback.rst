
:mod:`traceback` --- Print or retrieve a stack traceback
========================================================

.. module:: traceback
   :synopsis: Print or retrieve a stack traceback.


This module provides a standard interface to extract, format and print stack
traces of Python programs.  It exactly mimics the behavior of the Python
interpreter when it prints a stack trace.  This is useful when you want to print
stack traces under program control, such as in a "wrapper" around the
interpreter.

.. index:: object: traceback

The module uses traceback objects --- this is the object type that is stored in
the ``sys.last_traceback`` variable and returned as the third item from
:func:`sys.exc_info`.

The module defines the following functions:


.. function:: print_tb(traceback[, limit[, file]])

   Print up to *limit* stack trace entries from *traceback*.  If *limit* is omitted
   or ``None``, all entries are printed. If *file* is omitted or ``None``, the
   output goes to ``sys.stderr``; otherwise it should be an open file or file-like
   object to receive the output.


.. function:: print_exception(type, value, traceback[, limit[, file]])

   Print exception information and up to *limit* stack trace entries from
   *traceback* to *file*. This differs from :func:`print_tb` in the following ways:
   (1) if *traceback* is not ``None``, it prints a header ``Traceback (most recent
   call last):``; (2) it prints the exception *type* and *value* after the stack
   trace; (3) if *type* is :exc:`SyntaxError` and *value* has the appropriate
   format, it prints the line where the syntax error occurred with a caret
   indicating the approximate position of the error.


.. function:: print_exc([limit[, file]])

   This is a shorthand for ``print_exception(*sys.exc_info()``.


.. function:: format_exc([limit])

   This is like ``print_exc(limit)`` but returns a string instead of printing to a
   file.


.. function:: print_last([limit[, file]])

   This is a shorthand for ``print_exception(sys.last_type, sys.last_value,
   sys.last_traceback, limit, file)``.


.. function:: print_stack([f[, limit[, file]]])

   This function prints a stack trace from its invocation point.  The optional *f*
   argument can be used to specify an alternate stack frame to start.  The optional
   *limit* and *file* arguments have the same meaning as for
   :func:`print_exception`.


.. function:: extract_tb(traceback[, limit])

   Return a list of up to *limit* "pre-processed" stack trace entries extracted
   from the traceback object *traceback*.  It is useful for alternate formatting of
   stack traces.  If *limit* is omitted or ``None``, all entries are extracted.  A
   "pre-processed" stack trace entry is a quadruple (*filename*, *line number*,
   *function name*, *text*) representing the information that is usually printed
   for a stack trace.  The *text* is a string with leading and trailing whitespace
   stripped; if the source is not available it is ``None``.


.. function:: extract_stack([f[, limit]])

   Extract the raw traceback from the current stack frame.  The return value has
   the same format as for :func:`extract_tb`.  The optional *f* and *limit*
   arguments have the same meaning as for :func:`print_stack`.


.. function:: format_list(list)

   Given a list of tuples as returned by :func:`extract_tb` or
   :func:`extract_stack`, return a list of strings ready for printing.  Each string
   in the resulting list corresponds to the item with the same index in the
   argument list.  Each string ends in a newline; the strings may contain internal
   newlines as well, for those items whose source text line is not ``None``.


.. function:: format_exception_only(type, value)

   Format the exception part of a traceback.  The arguments are the exception type
   and value such as given by ``sys.last_type`` and ``sys.last_value``.  The return
   value is a list of strings, each ending in a newline.  Normally, the list
   contains a single string; however, for :exc:`SyntaxError` exceptions, it
   contains several lines that (when printed) display detailed information about
   where the syntax error occurred.  The message indicating which exception
   occurred is the always last string in the list.


.. function:: format_exception(type, value, tb[, limit])

   Format a stack trace and the exception information.  The arguments  have the
   same meaning as the corresponding arguments to :func:`print_exception`.  The
   return value is a list of strings, each ending in a newline and some containing
   internal newlines.  When these lines are concatenated and printed, exactly the
   same text is printed as does :func:`print_exception`.


.. function:: format_tb(tb[, limit])

   A shorthand for ``format_list(extract_tb(tb, limit))``.


.. function:: format_stack([f[, limit]])

   A shorthand for ``format_list(extract_stack(f, limit))``.


.. function:: tb_lineno(tb)

   This function returns the current line number set in the traceback object.  This
   function was necessary because in versions of Python prior to 2.3 when the
   :option:`-O` flag was passed to Python the ``tb.tb_lineno`` was not updated
   correctly.  This function has no use in versions past 2.3.


.. _traceback-example:

Traceback Example
-----------------

This simple example implements a basic read-eval-print loop, similar to (but
less useful than) the standard Python interactive interpreter loop.  For a more
complete implementation of the interpreter loop, refer to the :mod:`code`
module. ::

   import sys, traceback

   def run_user_code(envdir):
       source = raw_input(">>> ")
       try:
           exec(source, envdir)
       except:
           print "Exception in user code:"
           print '-'*60
           traceback.print_exc(file=sys.stdout)
           print '-'*60

   envdir = {}
   while 1:
       run_user_code(envdir)

