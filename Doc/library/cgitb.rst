
:mod:`cgitb` --- Traceback manager for CGI scripts
==================================================

.. module:: cgitb
   :synopsis: Configurable traceback handler for CGI scripts.
.. moduleauthor:: Ka-Ping Yee <ping@lfw.org>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


.. versionadded:: 2.2

.. index::
   single: CGI; exceptions
   single: CGI; tracebacks
   single: exceptions; in CGI scripts
   single: tracebacks; in CGI scripts

The :mod:`cgitb` module provides a special exception handler for Python scripts.
(Its name is a bit misleading.  It was originally designed to display extensive
traceback information in HTML for CGI scripts.  It was later generalized to also
display this information in plain text.)  After this module is activated, if an
uncaught exception occurs, a detailed, formatted report will be displayed.  The
report includes a traceback showing excerpts of the source code for each level,
as well as the values of the arguments and local variables to currently running
functions, to help you debug the problem.  Optionally, you can save this
information to a file instead of sending it to the browser.

To enable this feature, simply add this to the top of your CGI script::

   import cgitb
   cgitb.enable()

The options to the :func:`enable` function control whether the report is
displayed in the browser and whether the report is logged to a file for later
analysis.


.. function:: enable([display[, logdir[, context[, format]]]])

   .. index:: single: excepthook() (in module sys)

   This function causes the :mod:`cgitb` module to take over the interpreter's
   default handling for exceptions by setting the value of :attr:`sys.excepthook`.

   The optional argument *display* defaults to ``1`` and can be set to ``0`` to
   suppress sending the traceback to the browser. If the argument *logdir* is
   present, the traceback reports are written to files.  The value of *logdir*
   should be a directory where these files will be placed. The optional argument
   *context* is the number of lines of context to display around the current line
   of source code in the traceback; this defaults to ``5``. If the optional
   argument *format* is ``"html"``, the output is formatted as HTML.  Any other
   value forces plain text output.  The default value is ``"html"``.


.. function:: handler([info])

   This function handles an exception using the default settings (that is, show a
   report in the browser, but don't log to a file). This can be used when you've
   caught an exception and want to report it using :mod:`cgitb`.  The optional
   *info* argument should be a 3-tuple containing an exception type, exception
   value, and traceback object, exactly like the tuple returned by
   :func:`sys.exc_info`.  If the *info* argument is not supplied, the current
   exception is obtained from :func:`sys.exc_info`.

