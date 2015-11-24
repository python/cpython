:mod:`logging` --- Logging facility for Python
==============================================

.. module:: logging
   :synopsis: Flexible event logging system for applications.


.. moduleauthor:: Vinay Sajip <vinay_sajip@red-dove.com>
.. sectionauthor:: Vinay Sajip <vinay_sajip@red-dove.com>


.. index:: pair: Errors; logging

.. sidebar:: Important

   This page contains the API reference information. For tutorial
   information and discussion of more advanced topics, see

   * :ref:`Basic Tutorial <logging-basic-tutorial>`
   * :ref:`Advanced Tutorial <logging-advanced-tutorial>`
   * :ref:`Logging Cookbook <logging-cookbook>`

**Source code:** :source:`Lib/logging/__init__.py`

--------------

.. versionadded:: 2.3

This module defines functions and classes which implement a flexible event
logging system for applications and libraries.

The key benefit of having the logging API provided by a standard library module
is that all Python modules can participate in logging, so your application log
can include your own messages integrated with messages from third-party
modules.

The module provides a lot of functionality and flexibility.  If you are
unfamiliar with logging, the best way to get to grips with it is to see the
tutorials (see the links on the right).

The basic classes defined by the module, together with their functions, are
listed below.

* Loggers expose the interface that application code directly uses.
* Handlers send the log records (created by loggers) to the appropriate
  destination.
* Filters provide a finer grained facility for determining which log records
  to output.
* Formatters specify the layout of log records in the final output.


.. _logger:

Logger Objects
--------------

Loggers have the following attributes and methods.  Note that Loggers are never
instantiated directly, but always through the module-level function
``logging.getLogger(name)``.  Multiple calls to :func:`getLogger` with the same
name will always return a reference to the same Logger object.

The ``name`` is potentially a period-separated hierarchical value, like
``foo.bar.baz`` (though it could also be just plain ``foo``, for example).
Loggers that are further down in the hierarchical list are children of loggers
higher up in the list.  For example, given a logger with a name of ``foo``,
loggers with names of ``foo.bar``, ``foo.bar.baz``, and ``foo.bam`` are all
descendants of ``foo``.  The logger name hierarchy is analogous to the Python
package hierarchy, and identical to it if you organise your loggers on a
per-module basis using the recommended construction
``logging.getLogger(__name__)``.  That's because in a module, ``__name__``
is the module's name in the Python package namespace.


.. class:: Logger

.. attribute:: Logger.propagate

   If this evaluates to true, events logged to this logger will be passed to the
   handlers of higher level (ancestor) loggers, in addition to any handlers
   attached to this logger. Messages are passed directly to the ancestor
   loggers' handlers - neither the level nor filters of the ancestor loggers in
   question are considered.

   If this evaluates to false, logging messages are not passed to the handlers
   of ancestor loggers.

   The constructor sets this attribute to ``True``.

   .. note:: If you attach a handler to a logger *and* one or more of its
      ancestors, it may emit the same record multiple times. In general, you
      should not need to attach a handler to more than one logger - if you just
      attach it to the appropriate logger which is highest in the logger
      hierarchy, then it will see all events logged by all descendant loggers,
      provided that their propagate setting is left set to ``True``. A common
      scenario is to attach handlers only to the root logger, and to let
      propagation take care of the rest.

.. method:: Logger.setLevel(lvl)

   Sets the threshold for this logger to *lvl*. Logging messages which are less
   severe than *lvl* will be ignored. When a logger is created, the level is set to
   :const:`NOTSET` (which causes all messages to be processed when the logger is
   the root logger, or delegation to the parent when the logger is a non-root
   logger). Note that the root logger is created with level :const:`WARNING`.

   The term 'delegation to the parent' means that if a logger has a level of
   NOTSET, its chain of ancestor loggers is traversed until either an ancestor with
   a level other than NOTSET is found, or the root is reached.

   If an ancestor is found with a level other than NOTSET, then that ancestor's
   level is treated as the effective level of the logger where the ancestor search
   began, and is used to determine how a logging event is handled.

   If the root is reached, and it has a level of NOTSET, then all messages will be
   processed. Otherwise, the root's level will be used as the effective level.

   See :ref:`levels` for a list of levels.


.. method:: Logger.isEnabledFor(lvl)

   Indicates if a message of severity *lvl* would be processed by this logger.
   This method checks first the module-level level set by
   ``logging.disable(lvl)`` and then the logger's effective level as determined
   by :meth:`getEffectiveLevel`.


.. method:: Logger.getEffectiveLevel()

   Indicates the effective level for this logger. If a value other than
   :const:`NOTSET` has been set using :meth:`setLevel`, it is returned. Otherwise,
   the hierarchy is traversed towards the root until a value other than
   :const:`NOTSET` is found, and that value is returned. The value returned is
   an integer, typically one of :const:`logging.DEBUG`, :const:`logging.INFO`
   etc.


.. method:: Logger.getChild(suffix)

   Returns a logger which is a descendant to this logger, as determined by the suffix.
   Thus, ``logging.getLogger('abc').getChild('def.ghi')`` would return the same
   logger as would be returned by ``logging.getLogger('abc.def.ghi')``. This is a
   convenience method, useful when the parent logger is named using e.g. ``__name__``
   rather than a literal string.

   .. versionadded:: 2.7


.. method:: Logger.debug(msg, *args, **kwargs)

   Logs a message with level :const:`DEBUG` on this logger. The *msg* is the
   message format string, and the *args* are the arguments which are merged into
   *msg* using the string formatting operator. (Note that this means that you can
   use keywords in the format string, together with a single dictionary argument.)

   There are two keyword arguments in *kwargs* which are inspected: *exc_info*
   which, if it does not evaluate as false, causes exception information to be
   added to the logging message. If an exception tuple (in the format returned by
   :func:`sys.exc_info`) is provided, it is used; otherwise, :func:`sys.exc_info`
   is called to get the exception information.

   The second keyword argument is *extra* which can be used to pass a
   dictionary which is used to populate the __dict__ of the LogRecord created for
   the logging event with user-defined attributes. These custom attributes can then
   be used as you like. For example, they could be incorporated into logged
   messages. For example::

      FORMAT = '%(asctime)-15s %(clientip)s %(user)-8s %(message)s'
      logging.basicConfig(format=FORMAT)
      d = {'clientip': '192.168.0.1', 'user': 'fbloggs'}
      logger = logging.getLogger('tcpserver')
      logger.warning('Protocol problem: %s', 'connection reset', extra=d)

   would print something like  ::

      2006-02-08 22:20:02,165 192.168.0.1 fbloggs  Protocol problem: connection reset

   The keys in the dictionary passed in *extra* should not clash with the keys used
   by the logging system. (See the :class:`Formatter` documentation for more
   information on which keys are used by the logging system.)

   If you choose to use these attributes in logged messages, you need to exercise
   some care. In the above example, for instance, the :class:`Formatter` has been
   set up with a format string which expects 'clientip' and 'user' in the attribute
   dictionary of the LogRecord. If these are missing, the message will not be
   logged because a string formatting exception will occur. So in this case, you
   always need to pass the *extra* dictionary with these keys.

   While this might be annoying, this feature is intended for use in specialized
   circumstances, such as multi-threaded servers where the same code executes in
   many contexts, and interesting conditions which arise are dependent on this
   context (such as remote client IP address and authenticated user name, in the
   above example). In such circumstances, it is likely that specialized
   :class:`Formatter`\ s would be used with particular :class:`Handler`\ s.


.. method:: Logger.info(msg, *args, **kwargs)

   Logs a message with level :const:`INFO` on this logger. The arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.warning(msg, *args, **kwargs)

   Logs a message with level :const:`WARNING` on this logger. The arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.error(msg, *args, **kwargs)

   Logs a message with level :const:`ERROR` on this logger. The arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.critical(msg, *args, **kwargs)

   Logs a message with level :const:`CRITICAL` on this logger. The arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.log(lvl, msg, *args, **kwargs)

   Logs a message with integer level *lvl* on this logger. The other arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.exception(msg, *args, **kwargs)

   Logs a message with level :const:`ERROR` on this logger. The arguments are
   interpreted as for :meth:`debug`, except that any passed *exc_info* is not
   inspected. Exception info is always added to the logging message. This method
   should only be called from an exception handler.


.. method:: Logger.addFilter(filt)

   Adds the specified filter *filt* to this logger.


.. method:: Logger.removeFilter(filt)

   Removes the specified filter *filt* from this logger.


.. method:: Logger.filter(record)

   Applies this logger's filters to the record and returns a true value if the
   record is to be processed. The filters are consulted in turn, until one of
   them returns a false value. If none of them return a false value, the record
   will be processed (passed to handlers). If one returns a false value, no
   further processing of the record occurs.


.. method:: Logger.addHandler(hdlr)

   Adds the specified handler *hdlr* to this logger.


.. method:: Logger.removeHandler(hdlr)

   Removes the specified handler *hdlr* from this logger.


.. method:: Logger.findCaller()

   Finds the caller's source filename and line number. Returns the filename, line
   number and function name as a 3-element tuple.

   .. versionchanged:: 2.4
      The function name was added. In earlier versions, the filename and line
      number were returned as a 2-element tuple.

.. method:: Logger.handle(record)

   Handles a record by passing it to all handlers associated with this logger and
   its ancestors (until a false value of *propagate* is found). This method is used
   for unpickled records received from a socket, as well as those created locally.
   Logger-level filtering is applied using :meth:`~Logger.filter`.


.. method:: Logger.makeRecord(name, lvl, fn, lno, msg, args, exc_info, func=None, extra=None)

   This is a factory method which can be overridden in subclasses to create
   specialized :class:`LogRecord` instances.

   .. versionchanged:: 2.5
      *func* and *extra* were added.


.. _levels:

Logging Levels
--------------

The numeric values of logging levels are given in the following table. These are
primarily of interest if you want to define your own levels, and need them to
have specific values relative to the predefined levels. If you define a level
with the same numeric value, it overwrites the predefined value; the predefined
name is lost.

+--------------+---------------+
| Level        | Numeric value |
+==============+===============+
| ``CRITICAL`` | 50            |
+--------------+---------------+
| ``ERROR``    | 40            |
+--------------+---------------+
| ``WARNING``  | 30            |
+--------------+---------------+
| ``INFO``     | 20            |
+--------------+---------------+
| ``DEBUG``    | 10            |
+--------------+---------------+
| ``NOTSET``   | 0             |
+--------------+---------------+


.. _handler:

Handler Objects
---------------

Handlers have the following attributes and methods. Note that :class:`Handler`
is never instantiated directly; this class acts as a base for more useful
subclasses. However, the :meth:`__init__` method in subclasses needs to call
:meth:`Handler.__init__`.


.. method:: Handler.__init__(level=NOTSET)

   Initializes the :class:`Handler` instance by setting its level, setting the list
   of filters to the empty list and creating a lock (using :meth:`createLock`) for
   serializing access to an I/O mechanism.


.. method:: Handler.createLock()

   Initializes a thread lock which can be used to serialize access to underlying
   I/O functionality which may not be threadsafe.


.. method:: Handler.acquire()

   Acquires the thread lock created with :meth:`createLock`.


.. method:: Handler.release()

   Releases the thread lock acquired with :meth:`acquire`.


.. method:: Handler.setLevel(lvl)

   Sets the threshold for this handler to *lvl*. Logging messages which are less
   severe than *lvl* will be ignored. When a handler is created, the level is set
   to :const:`NOTSET` (which causes all messages to be processed).

   See :ref:`levels` for a list of levels.

.. method:: Handler.setFormatter(form)

   Sets the :class:`Formatter` for this handler to *form*.


.. method:: Handler.addFilter(filt)

   Adds the specified filter *filt* to this handler.


.. method:: Handler.removeFilter(filt)

   Removes the specified filter *filt* from this handler.


.. method:: Handler.filter(record)

   Applies this handler's filters to the record and returns a true value if the
   record is to be processed. The filters are consulted in turn, until one of
   them returns a false value. If none of them return a false value, the record
   will be emitted. If one returns a false value, the handler will not emit the
   record.


.. method:: Handler.flush()

   Ensure all logging output has been flushed. This version does nothing and is
   intended to be implemented by subclasses.


.. method:: Handler.close()

   Tidy up any resources used by the handler. This version does no output but
   removes the handler from an internal list of handlers which is closed when
   :func:`shutdown` is called. Subclasses should ensure that this gets called
   from overridden :meth:`close` methods.


.. method:: Handler.handle(record)

   Conditionally emits the specified logging record, depending on filters which may
   have been added to the handler. Wraps the actual emission of the record with
   acquisition/release of the I/O thread lock.


.. method:: Handler.handleError(record)

   This method should be called from handlers when an exception is encountered
   during an :meth:`emit` call. If the module-level attribute
   ``raiseExceptions`` is ``False``, exceptions get silently ignored. This is
   what is mostly wanted for a logging system - most users will not care about
   errors in the logging system, they are more interested in application
   errors. You could, however, replace this with a custom handler if you wish.
   The specified record is the one which was being processed when the exception
   occurred. (The default value of ``raiseExceptions`` is ``True``, as that is
   more useful during development).


.. method:: Handler.format(record)

   Do formatting for a record - if a formatter is set, use it. Otherwise, use the
   default formatter for the module.


.. method:: Handler.emit(record)

   Do whatever it takes to actually log the specified logging record. This version
   is intended to be implemented by subclasses and so raises a
   :exc:`NotImplementedError`.

For a list of handlers included as standard, see :mod:`logging.handlers`.

.. _formatter-objects:

Formatter Objects
-----------------

.. currentmodule:: logging

:class:`Formatter` objects have the following attributes and methods. They are
responsible for converting a :class:`LogRecord` to (usually) a string which can
be interpreted by either a human or an external system. The base
:class:`Formatter` allows a formatting string to be specified. If none is
supplied, the default value of ``'%(message)s'`` is used, which just includes
the message in the logging call. To have additional items of information in the
formatted output (such as a timestamp), keep reading.

A Formatter can be initialized with a format string which makes use of knowledge
of the :class:`LogRecord` attributes - such as the default value mentioned above
making use of the fact that the user's message and arguments are pre-formatted
into a :class:`LogRecord`'s *message* attribute.  This format string contains
standard Python %-style mapping keys. See section :ref:`string-formatting`
for more information on string formatting.

The useful mapping keys in a :class:`LogRecord` are given in the section on
:ref:`logrecord-attributes`.


.. class:: Formatter(fmt=None, datefmt=None)

   Returns a new instance of the :class:`Formatter` class.  The instance is
   initialized with a format string for the message as a whole, as well as a
   format string for the date/time portion of a message.  If no *fmt* is
   specified, ``'%(message)s'`` is used.  If no *datefmt* is specified, the
   ISO8601 date format is used.

   .. method:: format(record)

      The record's attribute dictionary is used as the operand to a string
      formatting operation. Returns the resulting string. Before formatting the
      dictionary, a couple of preparatory steps are carried out. The *message*
      attribute of the record is computed using *msg* % *args*. If the
      formatting string contains ``'(asctime)'``, :meth:`formatTime` is called
      to format the event time. If there is exception information, it is
      formatted using :meth:`formatException` and appended to the message. Note
      that the formatted exception information is cached in attribute
      *exc_text*. This is useful because the exception information can be
      pickled and sent across the wire, but you should be careful if you have
      more than one :class:`Formatter` subclass which customizes the formatting
      of exception information. In this case, you will have to clear the cached
      value after a formatter has done its formatting, so that the next
      formatter to handle the event doesn't use the cached value but
      recalculates it afresh.


   .. method:: formatTime(record, datefmt=None)

      This method should be called from :meth:`format` by a formatter which
      wants to make use of a formatted time. This method can be overridden in
      formatters to provide for any specific requirement, but the basic behavior
      is as follows: if *datefmt* (a string) is specified, it is used with
      :func:`time.strftime` to format the creation time of the
      record. Otherwise, the ISO8601 format is used.  The resulting string is
      returned.

      This function uses a user-configurable function to convert the creation
      time to a tuple. By default, :func:`time.localtime` is used; to change
      this for a particular formatter instance, set the ``converter`` attribute
      to a function with the same signature as :func:`time.localtime` or
      :func:`time.gmtime`. To change it for all formatters, for example if you
      want all logging times to be shown in GMT, set the ``converter``
      attribute in the ``Formatter`` class.

   .. method:: formatException(exc_info)

      Formats the specified exception information (a standard exception tuple as
      returned by :func:`sys.exc_info`) as a string. This default implementation
      just uses :func:`traceback.print_exception`. The resulting string is
      returned.

.. _filter:

Filter Objects
--------------

``Filters`` can be used by ``Handlers`` and ``Loggers`` for more sophisticated
filtering than is provided by levels. The base filter class only allows events
which are below a certain point in the logger hierarchy. For example, a filter
initialized with 'A.B' will allow events logged by loggers 'A.B', 'A.B.C',
'A.B.C.D', 'A.B.D' etc. but not 'A.BB', 'B.A.B' etc. If initialized with the
empty string, all events are passed.


.. class:: Filter(name='')

   Returns an instance of the :class:`Filter` class. If *name* is specified, it
   names a logger which, together with its children, will have its events allowed
   through the filter. If *name* is the empty string, allows every event.


   .. method:: filter(record)

      Is the specified record to be logged? Returns zero for no, nonzero for
      yes. If deemed appropriate, the record may be modified in-place by this
      method.

Note that filters attached to handlers are consulted before an event is
emitted by the handler, whereas filters attached to loggers are consulted
whenever an event is logged (using :meth:`debug`, :meth:`info`,
etc.), before sending an event to handlers. This means that events which have
been generated by descendant loggers will not be filtered by a logger's filter
setting, unless the filter has also been applied to those descendant loggers.

You don't actually need to subclass ``Filter``: you can pass any instance
which has a ``filter`` method with the same semantics.

Although filters are used primarily to filter records based on more
sophisticated criteria than levels, they get to see every record which is
processed by the handler or logger they're attached to: this can be useful if
you want to do things like counting how many records were processed by a
particular logger or handler, or adding, changing or removing attributes in
the LogRecord being processed. Obviously changing the LogRecord needs to be
done with some care, but it does allow the injection of contextual information
into logs (see :ref:`filters-contextual`).

.. _log-record:

LogRecord Objects
-----------------

:class:`LogRecord` instances are created automatically by the :class:`Logger`
every time something is logged, and can be created manually via
:func:`makeLogRecord` (for example, from a pickled event received over the
wire).


.. class:: LogRecord(name, level, pathname, lineno, msg, args, exc_info, func=None)

   Contains all the information pertinent to the event being logged.

   The primary information is passed in :attr:`msg` and :attr:`args`, which
   are combined using ``msg % args`` to create the :attr:`message` field of the
   record.

   :param name:  The name of the logger used to log the event represented by
                 this LogRecord. Note that this name will always have this
                 value, even though it may be emitted by a handler attached to
                 a different (ancestor) logger.
   :param level: The numeric level of the logging event (one of DEBUG, INFO etc.)
                 Note that this is converted to *two* attributes of the LogRecord:
                 ``levelno`` for the numeric value and ``levelname`` for the
                 corresponding level name.
   :param pathname: The full pathname of the source file where the logging call
                    was made.
   :param lineno: The line number in the source file where the logging call was
                  made.
   :param msg: The event description message, possibly a format string with
               placeholders for variable data.
   :param args: Variable data to merge into the *msg* argument to obtain the
                event description.
   :param exc_info: An exception tuple with the current exception information,
                    or *None* if no exception information is available.
   :param func: The name of the function or method from which the logging call
                was invoked.

   .. versionchanged:: 2.5
      *func* was added.

   .. method:: getMessage()

      Returns the message for this :class:`LogRecord` instance after merging any
      user-supplied arguments with the message. If the user-supplied message
      argument to the logging call is not a string, :func:`str` is called on it to
      convert it to a string. This allows use of user-defined classes as
      messages, whose ``__str__`` method can return the actual format string to
      be used.


.. _logrecord-attributes:

LogRecord attributes
--------------------

The LogRecord has a number of attributes, most of which are derived from the
parameters to the constructor. (Note that the names do not always correspond
exactly between the LogRecord constructor parameters and the LogRecord
attributes.) These attributes can be used to merge data from the record into
the format string. The following table lists (in alphabetical order) the
attribute names, their meanings and the corresponding placeholder in a %-style
format string.

+----------------+-------------------------+-----------------------------------------------+
| Attribute name | Format                  | Description                                   |
+================+=========================+===============================================+
| args           | You shouldn't need to   | The tuple of arguments merged into ``msg`` to |
|                | format this yourself.   | produce ``message``, or a dict whose values   |
|                |                         | are used for the merge (when there is only one|
|                |                         | argument, and it is a dictionary).            |
+----------------+-------------------------+-----------------------------------------------+
| asctime        | ``%(asctime)s``         | Human-readable time when the                  |
|                |                         | :class:`LogRecord` was created.  By default   |
|                |                         | this is of the form '2003-07-08 16:49:45,896' |
|                |                         | (the numbers after the comma are millisecond  |
|                |                         | portion of the time).                         |
+----------------+-------------------------+-----------------------------------------------+
| created        | ``%(created)f``         | Time when the :class:`LogRecord` was created  |
|                |                         | (as returned by :func:`time.time`).           |
+----------------+-------------------------+-----------------------------------------------+
| exc_info       | You shouldn't need to   | Exception tuple (à la ``sys.exc_info``) or,   |
|                | format this yourself.   | if no exception has occurred, *None*.         |
+----------------+-------------------------+-----------------------------------------------+
| filename       | ``%(filename)s``        | Filename portion of ``pathname``.             |
+----------------+-------------------------+-----------------------------------------------+
| funcName       | ``%(funcName)s``        | Name of function containing the logging call. |
+----------------+-------------------------+-----------------------------------------------+
| levelname      | ``%(levelname)s``       | Text logging level for the message            |
|                |                         | (``'DEBUG'``, ``'INFO'``, ``'WARNING'``,      |
|                |                         | ``'ERROR'``, ``'CRITICAL'``).                 |
+----------------+-------------------------+-----------------------------------------------+
| levelno        | ``%(levelno)s``         | Numeric logging level for the message         |
|                |                         | (:const:`DEBUG`, :const:`INFO`,               |
|                |                         | :const:`WARNING`, :const:`ERROR`,             |
|                |                         | :const:`CRITICAL`).                           |
+----------------+-------------------------+-----------------------------------------------+
| lineno         | ``%(lineno)d``          | Source line number where the logging call was |
|                |                         | issued (if available).                        |
+----------------+-------------------------+-----------------------------------------------+
| module         | ``%(module)s``          | Module (name portion of ``filename``).        |
+----------------+-------------------------+-----------------------------------------------+
| msecs          | ``%(msecs)d``           | Millisecond portion of the time when the      |
|                |                         | :class:`LogRecord` was created.               |
+----------------+-------------------------+-----------------------------------------------+
| message        | ``%(message)s``         | The logged message, computed as ``msg %       |
|                |                         | args``. This is set when                      |
|                |                         | :meth:`Formatter.format` is invoked.          |
+----------------+-------------------------+-----------------------------------------------+
| msg            | You shouldn't need to   | The format string passed in the original      |
|                | format this yourself.   | logging call. Merged with ``args`` to         |
|                |                         | produce ``message``, or an arbitrary object   |
|                |                         | (see :ref:`arbitrary-object-messages`).       |
+----------------+-------------------------+-----------------------------------------------+
| name           | ``%(name)s``            | Name of the logger used to log the call.      |
+----------------+-------------------------+-----------------------------------------------+
| pathname       | ``%(pathname)s``        | Full pathname of the source file where the    |
|                |                         | logging call was issued (if available).       |
+----------------+-------------------------+-----------------------------------------------+
| process        | ``%(process)d``         | Process ID (if available).                    |
+----------------+-------------------------+-----------------------------------------------+
| processName    | ``%(processName)s``     | Process name (if available).                  |
+----------------+-------------------------+-----------------------------------------------+
| relativeCreated| ``%(relativeCreated)d`` | Time in milliseconds when the LogRecord was   |
|                |                         | created, relative to the time the logging     |
|                |                         | module was loaded.                            |
+----------------+-------------------------+-----------------------------------------------+
| thread         | ``%(thread)d``          | Thread ID (if available).                     |
+----------------+-------------------------+-----------------------------------------------+
| threadName     | ``%(threadName)s``      | Thread name (if available).                   |
+----------------+-------------------------+-----------------------------------------------+

.. versionchanged:: 2.5
   *funcName* was added.

.. versionchanged:: 2.6
   *processName* was added.

.. _logger-adapter:

LoggerAdapter Objects
---------------------

:class:`LoggerAdapter` instances are used to conveniently pass contextual
information into logging calls. For a usage example, see the section on
:ref:`adding contextual information to your logging output <context-info>`.

.. versionadded:: 2.6


.. class:: LoggerAdapter(logger, extra)

   Returns an instance of :class:`LoggerAdapter` initialized with an
   underlying :class:`Logger` instance and a dict-like object.

   .. method:: process(msg, kwargs)

      Modifies the message and/or keyword arguments passed to a logging call in
      order to insert contextual information. This implementation takes the object
      passed as *extra* to the constructor and adds it to *kwargs* using key
      'extra'. The return value is a (*msg*, *kwargs*) tuple which has the
      (possibly modified) versions of the arguments passed in.

In addition to the above, :class:`LoggerAdapter` supports the following
methods of :class:`Logger`: :meth:`~Logger.debug`, :meth:`~Logger.info`,
:meth:`~Logger.warning`, :meth:`~Logger.error`, :meth:`~Logger.exception`,
:meth:`~Logger.critical`, :meth:`~Logger.log` and :meth:`~Logger.isEnabledFor`.
These methods have the same signatures as their counterparts in :class:`Logger`,
so you can use the two types of instances interchangeably for these calls.

.. versionchanged:: 2.7
   The :meth:`~Logger.isEnabledFor` method was added to :class:`LoggerAdapter`.
   This method delegates to the underlying logger.


Thread Safety
-------------

The logging module is intended to be thread-safe without any special work
needing to be done by its clients. It achieves this though using threading
locks; there is one lock to serialize access to the module's shared data, and
each handler also creates a lock to serialize access to its underlying I/O.

If you are implementing asynchronous signal handlers using the :mod:`signal`
module, you may not be able to use logging from within such handlers. This is
because lock implementations in the :mod:`threading` module are not always
re-entrant, and so cannot be invoked from such signal handlers.


Module-Level Functions
----------------------

In addition to the classes described above, there are a number of module- level
functions.


.. function:: getLogger([name])

   Return a logger with the specified name or, if no name is specified, return a
   logger which is the root logger of the hierarchy. If specified, the name is
   typically a dot-separated hierarchical name like *"a"*, *"a.b"* or *"a.b.c.d"*.
   Choice of these names is entirely up to the developer who is using logging.

   All calls to this function with a given name return the same logger instance.
   This means that logger instances never need to be passed between different parts
   of an application.


.. function:: getLoggerClass()

   Return either the standard :class:`Logger` class, or the last class passed to
   :func:`setLoggerClass`. This function may be called from within a new class
   definition, to ensure that installing a customized :class:`Logger` class will
   not undo customizations already applied by other code. For example::

      class MyLogger(logging.getLoggerClass()):
          # ... override behaviour here


.. function:: debug(msg[, *args[, **kwargs]])

   Logs a message with level :const:`DEBUG` on the root logger. The *msg* is the
   message format string, and the *args* are the arguments which are merged into
   *msg* using the string formatting operator. (Note that this means that you can
   use keywords in the format string, together with a single dictionary argument.)

   There are two keyword arguments in *kwargs* which are inspected: *exc_info*
   which, if it does not evaluate as false, causes exception information to be
   added to the logging message. If an exception tuple (in the format returned by
   :func:`sys.exc_info`) is provided, it is used; otherwise, :func:`sys.exc_info`
   is called to get the exception information.

   The other optional keyword argument is *extra* which can be used to pass a
   dictionary which is used to populate the __dict__ of the LogRecord created for
   the logging event with user-defined attributes. These custom attributes can then
   be used as you like. For example, they could be incorporated into logged
   messages. For example::

      FORMAT = "%(asctime)-15s %(clientip)s %(user)-8s %(message)s"
      logging.basicConfig(format=FORMAT)
      d = {'clientip': '192.168.0.1', 'user': 'fbloggs'}
      logging.warning("Protocol problem: %s", "connection reset", extra=d)

   would print something like::

      2006-02-08 22:20:02,165 192.168.0.1 fbloggs  Protocol problem: connection reset

   The keys in the dictionary passed in *extra* should not clash with the keys used
   by the logging system. (See the :class:`Formatter` documentation for more
   information on which keys are used by the logging system.)

   If you choose to use these attributes in logged messages, you need to exercise
   some care. In the above example, for instance, the :class:`Formatter` has been
   set up with a format string which expects 'clientip' and 'user' in the attribute
   dictionary of the LogRecord. If these are missing, the message will not be
   logged because a string formatting exception will occur. So in this case, you
   always need to pass the *extra* dictionary with these keys.

   While this might be annoying, this feature is intended for use in specialized
   circumstances, such as multi-threaded servers where the same code executes in
   many contexts, and interesting conditions which arise are dependent on this
   context (such as remote client IP address and authenticated user name, in the
   above example). In such circumstances, it is likely that specialized
   :class:`Formatter`\ s would be used with particular :class:`Handler`\ s.

   .. versionchanged:: 2.5
      *extra* was added.


.. function:: info(msg[, *args[, **kwargs]])

   Logs a message with level :const:`INFO` on the root logger. The arguments are
   interpreted as for :func:`debug`.


.. function:: warning(msg[, *args[, **kwargs]])

   Logs a message with level :const:`WARNING` on the root logger. The arguments are
   interpreted as for :func:`debug`.


.. function:: error(msg[, *args[, **kwargs]])

   Logs a message with level :const:`ERROR` on the root logger. The arguments are
   interpreted as for :func:`debug`.


.. function:: critical(msg[, *args[, **kwargs]])

   Logs a message with level :const:`CRITICAL` on the root logger. The arguments
   are interpreted as for :func:`debug`.


.. function:: exception(msg[, *args[, **kwargs]])

   Logs a message with level :const:`ERROR` on the root logger. The arguments are
   interpreted as for :func:`debug`, except that any passed *exc_info* is not
   inspected. Exception info is always added to the logging message. This
   function should only be called from an exception handler.


.. function:: log(level, msg[, *args[, **kwargs]])

   Logs a message with level *level* on the root logger. The other arguments are
   interpreted as for :func:`debug`.

   .. note:: The above module-level convenience functions, which delegate to the
      root logger, call :func:`basicConfig` to ensure that at least one handler
      is available. Because of this, they should *not* be used in threads,
      in versions of Python earlier than 2.7.1 and 3.2, unless at least one
      handler has been added to the root logger *before* the threads are
      started. In earlier versions of Python, due to a thread safety shortcoming
      in :func:`basicConfig`, this can (under rare circumstances) lead to
      handlers being added multiple times to the root logger, which can in turn
      lead to multiple messages for the same event.

.. function:: disable(lvl)

   Provides an overriding level *lvl* for all loggers which takes precedence over
   the logger's own level. When the need arises to temporarily throttle logging
   output down across the whole application, this function can be useful. Its
   effect is to disable all logging calls of severity *lvl* and below, so that
   if you call it with a value of INFO, then all INFO and DEBUG events would be
   discarded, whereas those of severity WARNING and above would be processed
   according to the logger's effective level. If
   ``logging.disable(logging.NOTSET)`` is called, it effectively removes this
   overriding level, so that logging output again depends on the effective
   levels of individual loggers.


.. function:: addLevelName(lvl, levelName)

   Associates level *lvl* with text *levelName* in an internal dictionary, which is
   used to map numeric levels to a textual representation, for example when a
   :class:`Formatter` formats a message. This function can also be used to define
   your own levels. The only constraints are that all levels used must be
   registered using this function, levels should be positive integers and they
   should increase in increasing order of severity.

   .. note:: If you are thinking of defining your own levels, please see the
      section on :ref:`custom-levels`.

.. function:: getLevelName(lvl)

   Returns the textual representation of logging level *lvl*. If the level is one
   of the predefined levels :const:`CRITICAL`, :const:`ERROR`, :const:`WARNING`,
   :const:`INFO` or :const:`DEBUG` then you get the corresponding string. If you
   have associated levels with names using :func:`addLevelName` then the name you
   have associated with *lvl* is returned. If a numeric value corresponding to one
   of the defined levels is passed in, the corresponding string representation is
   returned. Otherwise, the string "Level %s" % lvl is returned.

   .. note:: Integer levels should be used when e.g. setting levels on instances
      of :class:`Logger` and handlers. This function is used to convert between
      an integer level and the level name displayed in the formatted log output
      by means of the ``%(levelname)s`` format specifier (see
      :ref:`logrecord-attributes`).


.. function:: makeLogRecord(attrdict)

   Creates and returns a new :class:`LogRecord` instance whose attributes are
   defined by *attrdict*. This function is useful for taking a pickled
   :class:`LogRecord` attribute dictionary, sent over a socket, and reconstituting
   it as a :class:`LogRecord` instance at the receiving end.


.. function:: basicConfig([**kwargs])

   Does basic configuration for the logging system by creating a
   :class:`StreamHandler` with a default :class:`Formatter` and adding it to the
   root logger. The functions :func:`debug`, :func:`info`, :func:`warning`,
   :func:`error` and :func:`critical` will call :func:`basicConfig` automatically
   if no handlers are defined for the root logger.

   This function does nothing if the root logger already has handlers
   configured for it.

   .. versionchanged:: 2.4
      Formerly, :func:`basicConfig` did not take any keyword arguments.

   .. note:: This function should be called from the main thread before other
      threads are started. In versions of Python prior to 2.7.1 and 3.2, if
      this function is called from multiple threads, it is possible (in rare
      circumstances) that a handler will be added to the root logger more than
      once, leading to unexpected results such as messages being duplicated in
      the log.

   The following keyword arguments are supported.

   .. tabularcolumns:: |l|L|

   +--------------+---------------------------------------------+
   | Format       | Description                                 |
   +==============+=============================================+
   | ``filename`` | Specifies that a FileHandler be created,    |
   |              | using the specified filename, rather than a |
   |              | StreamHandler.                              |
   +--------------+---------------------------------------------+
   | ``filemode`` | Specifies the mode to open the file, if     |
   |              | filename is specified (if filemode is       |
   |              | unspecified, it defaults to 'a').           |
   +--------------+---------------------------------------------+
   | ``format``   | Use the specified format string for the     |
   |              | handler.                                    |
   +--------------+---------------------------------------------+
   | ``datefmt``  | Use the specified date/time format.         |
   +--------------+---------------------------------------------+
   | ``level``    | Set the root logger level to the specified  |
   |              | level.                                      |
   +--------------+---------------------------------------------+
   | ``stream``   | Use the specified stream to initialize the  |
   |              | StreamHandler. Note that this argument is   |
   |              | incompatible with 'filename' - if both are  |
   |              | present, 'stream' is ignored.               |
   +--------------+---------------------------------------------+


.. function:: shutdown()

   Informs the logging system to perform an orderly shutdown by flushing and
   closing all handlers. This should be called at application exit and no
   further use of the logging system should be made after this call.


.. function:: setLoggerClass(klass)

   Tells the logging system to use the class *klass* when instantiating a logger.
   The class should define :meth:`__init__` such that only a name argument is
   required, and the :meth:`__init__` should call :meth:`Logger.__init__`. This
   function is typically called before any loggers are instantiated by applications
   which need to use custom logger behavior.


Integration with the warnings module
------------------------------------

The :func:`captureWarnings` function can be used to integrate :mod:`logging`
with the :mod:`warnings` module.

.. function:: captureWarnings(capture)

   This function is used to turn the capture of warnings by logging on and
   off.

   If *capture* is ``True``, warnings issued by the :mod:`warnings` module will
   be redirected to the logging system. Specifically, a warning will be
   formatted using :func:`warnings.formatwarning` and the resulting string
   logged to a logger named ``'py.warnings'`` with a severity of :const:`WARNING`.

   If *capture* is ``False``, the redirection of warnings to the logging system
   will stop, and warnings will be redirected to their original destinations
   (i.e. those in effect before ``captureWarnings(True)`` was called).


.. seealso::

   Module :mod:`logging.config`
      Configuration API for the logging module.

   Module :mod:`logging.handlers`
      Useful handlers included with the logging module.

   :pep:`282` - A Logging System
      The proposal which described this feature for inclusion in the Python standard
      library.

   `Original Python logging package <http://www.red-dove.com/python_logging.html>`_
      This is the original source for the :mod:`logging` package.  The version of the
      package available from this site is suitable for use with Python 1.5.2, 2.1.x
      and 2.2.x, which do not include the :mod:`logging` package in the standard
      library.

