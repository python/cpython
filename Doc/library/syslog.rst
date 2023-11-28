:mod:`syslog` --- Unix syslog library routines
==============================================

.. module:: syslog
   :platform: Unix
   :synopsis: An interface to the Unix syslog library routines.

--------------

This module provides an interface to the Unix ``syslog`` library routines.
Refer to the Unix manual pages for a detailed description of the ``syslog``
facility.

.. availability:: Unix, not Emscripten, not WASI.

This module wraps the system ``syslog`` family of routines.  A pure Python
library that can speak to a syslog server is available in the
:mod:`logging.handlers` module as :class:`SysLogHandler`.

The module defines the following functions:


.. function:: syslog(message)
              syslog(priority, message)

   Send the string *message* to the system logger.  A trailing newline is added
   if necessary.  Each message is tagged with a priority composed of a
   *facility* and a *level*.  The optional *priority* argument, which defaults
   to :const:`LOG_INFO`, determines the message priority.  If the facility is
   not encoded in *priority* using logical-or (``LOG_INFO | LOG_USER``), the
   value given in the :func:`openlog` call is used.

   If :func:`openlog` has not been called prior to the call to :func:`syslog`,
   :func:`openlog` will be called with no arguments.

   .. audit-event:: syslog.syslog priority,message syslog.syslog

   .. versionchanged:: 3.2
      In previous versions, :func:`openlog` would not be called automatically if
      it wasn't called prior to the call to :func:`syslog`, deferring to the syslog
      implementation to call ``openlog()``.


.. function:: openlog([ident[, logoption[, facility]]])

   Logging options of subsequent :func:`syslog` calls can be set by calling
   :func:`openlog`.  :func:`syslog` will call :func:`openlog` with no arguments
   if the log is not currently open.

   The optional *ident* keyword argument is a string which is prepended to every
   message, and defaults to ``sys.argv[0]`` with leading path components
   stripped.  The optional *logoption* keyword argument (default is 0) is a bit
   field -- see below for possible values to combine.  The optional *facility*
   keyword argument (default is :const:`LOG_USER`) sets the default facility for
   messages which do not have a facility explicitly encoded.

   .. audit-event:: syslog.openlog ident,logoption,facility syslog.openlog

   .. versionchanged:: 3.2
      In previous versions, keyword arguments were not allowed, and *ident* was
      required.


.. function:: closelog()

   Reset the syslog module values and call the system library ``closelog()``.

   This causes the module to behave as it does when initially imported.  For
   example, :func:`openlog` will be called on the first :func:`syslog` call (if
   :func:`openlog` hasn't already been called), and *ident* and other
   :func:`openlog` parameters are reset to defaults.

   .. audit-event:: syslog.closelog "" syslog.closelog


.. function:: setlogmask(maskpri)

   Set the priority mask to *maskpri* and return the previous mask value.  Calls
   to :func:`syslog` with a priority level not set in *maskpri* are ignored.
   The default is to log all priorities.  The function ``LOG_MASK(pri)``
   calculates the mask for the individual priority *pri*.  The function
   ``LOG_UPTO(pri)`` calculates the mask for all priorities up to and including
   *pri*.

   .. audit-event:: syslog.setlogmask maskpri syslog.setlogmask

The module defines the following constants:

Priority levels (high to low):
   :const:`LOG_EMERG`, :const:`LOG_ALERT`, :const:`LOG_CRIT`, :const:`LOG_ERR`,
   :const:`LOG_WARNING`, :const:`LOG_NOTICE`, :const:`LOG_INFO`,
   :const:`LOG_DEBUG`.

Facilities:
   :const:`LOG_KERN`, :const:`LOG_USER`, :const:`LOG_MAIL`, :const:`LOG_DAEMON`,
   :const:`LOG_AUTH`, :const:`LOG_LPR`, :const:`LOG_NEWS`, :const:`LOG_UUCP`,
   :const:`LOG_CRON`, :const:`LOG_SYSLOG`, :const:`LOG_LOCAL0` to
   :const:`LOG_LOCAL7`, and, if defined in ``<syslog.h>``,
   :const:`LOG_AUTHPRIV`.

Log options:
   :const:`LOG_PID`, :const:`LOG_CONS`, :const:`LOG_NDELAY`, and, if defined
   in ``<syslog.h>``, :const:`LOG_ODELAY`, :const:`LOG_NOWAIT`, and
   :const:`LOG_PERROR`.


Examples
--------

Simple example
~~~~~~~~~~~~~~

A simple set of examples::

   import syslog

   syslog.syslog('Processing started')
   if error:
       syslog.syslog(syslog.LOG_ERR, 'Processing started')

An example of setting some log options, these would include the process ID in
logged messages, and write the messages to the destination facility used for
mail logging::

   syslog.openlog(logoption=syslog.LOG_PID, facility=syslog.LOG_MAIL)
   syslog.syslog('E-mail processing initiated...')
