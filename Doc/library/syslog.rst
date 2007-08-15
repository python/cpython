
:mod:`syslog` --- Unix syslog library routines
==============================================

.. module:: syslog
   :platform: Unix
   :synopsis: An interface to the Unix syslog library routines.


This module provides an interface to the Unix ``syslog`` library routines.
Refer to the Unix manual pages for a detailed description of the ``syslog``
facility.

The module defines the following functions:


.. function:: syslog([priority,] message)

   Send the string *message* to the system logger.  A trailing newline is added if
   necessary.  Each message is tagged with a priority composed of a *facility* and
   a *level*.  The optional *priority* argument, which defaults to
   :const:`LOG_INFO`, determines the message priority.  If the facility is not
   encoded in *priority* using logical-or (``LOG_INFO | LOG_USER``), the value
   given in the :func:`openlog` call is used.


.. function:: openlog(ident[, logopt[, facility]])

   Logging options other than the defaults can be set by explicitly opening the log
   file with :func:`openlog` prior to calling :func:`syslog`.  The defaults are
   (usually) *ident* = ``'syslog'``, *logopt* = ``0``, *facility* =
   :const:`LOG_USER`.  The *ident* argument is a string which is prepended to every
   message.  The optional *logopt* argument is a bit field - see below for possible
   values to combine.  The optional *facility* argument sets the default facility
   for messages which do not have a facility explicitly encoded.


.. function:: closelog()

   Close the log file.


.. function:: setlogmask(maskpri)

   Set the priority mask to *maskpri* and return the previous mask value.  Calls to
   :func:`syslog` with a priority level not set in *maskpri* are ignored.  The
   default is to log all priorities.  The function ``LOG_MASK(pri)`` calculates the
   mask for the individual priority *pri*.  The function ``LOG_UPTO(pri)``
   calculates the mask for all priorities up to and including *pri*.

The module defines the following constants:

Priority levels (high to low):
   :const:`LOG_EMERG`, :const:`LOG_ALERT`, :const:`LOG_CRIT`, :const:`LOG_ERR`,
   :const:`LOG_WARNING`, :const:`LOG_NOTICE`, :const:`LOG_INFO`,
   :const:`LOG_DEBUG`.

Facilities:
   :const:`LOG_KERN`, :const:`LOG_USER`, :const:`LOG_MAIL`, :const:`LOG_DAEMON`,
   :const:`LOG_AUTH`, :const:`LOG_LPR`, :const:`LOG_NEWS`, :const:`LOG_UUCP`,
   :const:`LOG_CRON` and :const:`LOG_LOCAL0` to :const:`LOG_LOCAL7`.

Log options:
   :const:`LOG_PID`, :const:`LOG_CONS`, :const:`LOG_NDELAY`, :const:`LOG_NOWAIT`
   and :const:`LOG_PERROR` if defined in ``<syslog.h>``.

