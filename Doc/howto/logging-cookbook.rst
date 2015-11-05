.. _logging-cookbook:

================
Logging Cookbook
================

:Author: Vinay Sajip <vinay_sajip at red-dove dot com>

This page contains a number of recipes related to logging, which have been found
useful in the past.

.. currentmodule:: logging

Using logging in multiple modules
---------------------------------

Multiple calls to ``logging.getLogger('someLogger')`` return a reference to the
same logger object.  This is true not only within the same module, but also
across modules as long as it is in the same Python interpreter process.  It is
true for references to the same object; additionally, application code can
define and configure a parent logger in one module and create (but not
configure) a child logger in a separate module, and all logger calls to the
child will pass up to the parent.  Here is a main module::

    import logging
    import auxiliary_module

    # create logger with 'spam_application'
    logger = logging.getLogger('spam_application')
    logger.setLevel(logging.DEBUG)
    # create file handler which logs even debug messages
    fh = logging.FileHandler('spam.log')
    fh.setLevel(logging.DEBUG)
    # create console handler with a higher log level
    ch = logging.StreamHandler()
    ch.setLevel(logging.ERROR)
    # create formatter and add it to the handlers
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    fh.setFormatter(formatter)
    ch.setFormatter(formatter)
    # add the handlers to the logger
    logger.addHandler(fh)
    logger.addHandler(ch)

    logger.info('creating an instance of auxiliary_module.Auxiliary')
    a = auxiliary_module.Auxiliary()
    logger.info('created an instance of auxiliary_module.Auxiliary')
    logger.info('calling auxiliary_module.Auxiliary.do_something')
    a.do_something()
    logger.info('finished auxiliary_module.Auxiliary.do_something')
    logger.info('calling auxiliary_module.some_function()')
    auxiliary_module.some_function()
    logger.info('done with auxiliary_module.some_function()')

Here is the auxiliary module::

    import logging

    # create logger
    module_logger = logging.getLogger('spam_application.auxiliary')

    class Auxiliary:
        def __init__(self):
            self.logger = logging.getLogger('spam_application.auxiliary.Auxiliary')
            self.logger.info('creating an instance of Auxiliary')
        def do_something(self):
            self.logger.info('doing something')
            a = 1 + 1
            self.logger.info('done doing something')

    def some_function():
        module_logger.info('received a call to "some_function"')

The output looks like this::

    2005-03-23 23:47:11,663 - spam_application - INFO -
       creating an instance of auxiliary_module.Auxiliary
    2005-03-23 23:47:11,665 - spam_application.auxiliary.Auxiliary - INFO -
       creating an instance of Auxiliary
    2005-03-23 23:47:11,665 - spam_application - INFO -
       created an instance of auxiliary_module.Auxiliary
    2005-03-23 23:47:11,668 - spam_application - INFO -
       calling auxiliary_module.Auxiliary.do_something
    2005-03-23 23:47:11,668 - spam_application.auxiliary.Auxiliary - INFO -
       doing something
    2005-03-23 23:47:11,669 - spam_application.auxiliary.Auxiliary - INFO -
       done doing something
    2005-03-23 23:47:11,670 - spam_application - INFO -
       finished auxiliary_module.Auxiliary.do_something
    2005-03-23 23:47:11,671 - spam_application - INFO -
       calling auxiliary_module.some_function()
    2005-03-23 23:47:11,672 - spam_application.auxiliary - INFO -
       received a call to 'some_function'
    2005-03-23 23:47:11,673 - spam_application - INFO -
       done with auxiliary_module.some_function()

Multiple handlers and formatters
--------------------------------

Loggers are plain Python objects.  The :meth:`~Logger.addHandler` method has no
minimum or maximum quota for the number of handlers you may add.  Sometimes it
will be beneficial for an application to log all messages of all severities to a
text file while simultaneously logging errors or above to the console.  To set
this up, simply configure the appropriate handlers.  The logging calls in the
application code will remain unchanged.  Here is a slight modification to the
previous simple module-based configuration example::

    import logging

    logger = logging.getLogger('simple_example')
    logger.setLevel(logging.DEBUG)
    # create file handler which logs even debug messages
    fh = logging.FileHandler('spam.log')
    fh.setLevel(logging.DEBUG)
    # create console handler with a higher log level
    ch = logging.StreamHandler()
    ch.setLevel(logging.ERROR)
    # create formatter and add it to the handlers
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    ch.setFormatter(formatter)
    fh.setFormatter(formatter)
    # add the handlers to logger
    logger.addHandler(ch)
    logger.addHandler(fh)

    # 'application' code
    logger.debug('debug message')
    logger.info('info message')
    logger.warn('warn message')
    logger.error('error message')
    logger.critical('critical message')

Notice that the 'application' code does not care about multiple handlers.  All
that changed was the addition and configuration of a new handler named *fh*.

The ability to create new handlers with higher- or lower-severity filters can be
very helpful when writing and testing an application.  Instead of using many
``print`` statements for debugging, use ``logger.debug``: Unlike the print
statements, which you will have to delete or comment out later, the logger.debug
statements can remain intact in the source code and remain dormant until you
need them again.  At that time, the only change that needs to happen is to
modify the severity level of the logger and/or handler to debug.

.. _multiple-destinations:

Logging to multiple destinations
--------------------------------

Let's say you want to log to console and file with different message formats and
in differing circumstances. Say you want to log messages with levels of DEBUG
and higher to file, and those messages at level INFO and higher to the console.
Let's also assume that the file should contain timestamps, but the console
messages should not. Here's how you can achieve this::

   import logging

   # set up logging to file - see previous section for more details
   logging.basicConfig(level=logging.DEBUG,
                       format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                       datefmt='%m-%d %H:%M',
                       filename='/temp/myapp.log',
                       filemode='w')
   # define a Handler which writes INFO messages or higher to the sys.stderr
   console = logging.StreamHandler()
   console.setLevel(logging.INFO)
   # set a format which is simpler for console use
   formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
   # tell the handler to use this format
   console.setFormatter(formatter)
   # add the handler to the root logger
   logging.getLogger('').addHandler(console)

   # Now, we can log to the root logger, or any other logger. First the root...
   logging.info('Jackdaws love my big sphinx of quartz.')

   # Now, define a couple of other loggers which might represent areas in your
   # application:

   logger1 = logging.getLogger('myapp.area1')
   logger2 = logging.getLogger('myapp.area2')

   logger1.debug('Quick zephyrs blow, vexing daft Jim.')
   logger1.info('How quickly daft jumping zebras vex.')
   logger2.warning('Jail zesty vixen who grabbed pay from quack.')
   logger2.error('The five boxing wizards jump quickly.')

When you run this, on the console you will see ::

   root        : INFO     Jackdaws love my big sphinx of quartz.
   myapp.area1 : INFO     How quickly daft jumping zebras vex.
   myapp.area2 : WARNING  Jail zesty vixen who grabbed pay from quack.
   myapp.area2 : ERROR    The five boxing wizards jump quickly.

and in the file you will see something like ::

   10-22 22:19 root         INFO     Jackdaws love my big sphinx of quartz.
   10-22 22:19 myapp.area1  DEBUG    Quick zephyrs blow, vexing daft Jim.
   10-22 22:19 myapp.area1  INFO     How quickly daft jumping zebras vex.
   10-22 22:19 myapp.area2  WARNING  Jail zesty vixen who grabbed pay from quack.
   10-22 22:19 myapp.area2  ERROR    The five boxing wizards jump quickly.

As you can see, the DEBUG message only shows up in the file. The other messages
are sent to both destinations.

This example uses console and file handlers, but you can use any number and
combination of handlers you choose.


Configuration server example
----------------------------

Here is an example of a module using the logging configuration server::

    import logging
    import logging.config
    import time
    import os

    # read initial config file
    logging.config.fileConfig('logging.conf')

    # create and start listener on port 9999
    t = logging.config.listen(9999)
    t.start()

    logger = logging.getLogger('simpleExample')

    try:
        # loop through logging calls to see the difference
        # new configurations make, until Ctrl+C is pressed
        while True:
            logger.debug('debug message')
            logger.info('info message')
            logger.warn('warn message')
            logger.error('error message')
            logger.critical('critical message')
            time.sleep(5)
    except KeyboardInterrupt:
        # cleanup
        logging.config.stopListening()
        t.join()

And here is a script that takes a filename and sends that file to the server,
properly preceded with the binary-encoded length, as the new logging
configuration::

    #!/usr/bin/env python
    import socket, sys, struct

    with open(sys.argv[1], 'rb') as f:
        data_to_send = f.read()

    HOST = 'localhost'
    PORT = 9999
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print('connecting...')
    s.connect((HOST, PORT))
    print('sending config...')
    s.send(struct.pack('>L', len(data_to_send)))
    s.send(data_to_send)
    s.close()
    print('complete')


.. _network-logging:

Sending and receiving logging events across a network
-----------------------------------------------------

Let's say you want to send logging events across a network, and handle them at
the receiving end. A simple way of doing this is attaching a
:class:`SocketHandler` instance to the root logger at the sending end::

   import logging, logging.handlers

   rootLogger = logging.getLogger('')
   rootLogger.setLevel(logging.DEBUG)
   socketHandler = logging.handlers.SocketHandler('localhost',
                       logging.handlers.DEFAULT_TCP_LOGGING_PORT)
   # don't bother with a formatter, since a socket handler sends the event as
   # an unformatted pickle
   rootLogger.addHandler(socketHandler)

   # Now, we can log to the root logger, or any other logger. First the root...
   logging.info('Jackdaws love my big sphinx of quartz.')

   # Now, define a couple of other loggers which might represent areas in your
   # application:

   logger1 = logging.getLogger('myapp.area1')
   logger2 = logging.getLogger('myapp.area2')

   logger1.debug('Quick zephyrs blow, vexing daft Jim.')
   logger1.info('How quickly daft jumping zebras vex.')
   logger2.warning('Jail zesty vixen who grabbed pay from quack.')
   logger2.error('The five boxing wizards jump quickly.')

At the receiving end, you can set up a receiver using the :mod:`SocketServer`
module. Here is a basic working example::

   import pickle
   import logging
   import logging.handlers
   import SocketServer
   import struct


   class LogRecordStreamHandler(SocketServer.StreamRequestHandler):
       """Handler for a streaming logging request.

       This basically logs the record using whatever logging policy is
       configured locally.
       """

       def handle(self):
           """
           Handle multiple requests - each expected to be a 4-byte length,
           followed by the LogRecord in pickle format. Logs the record
           according to whatever policy is configured locally.
           """
           while True:
               chunk = self.connection.recv(4)
               if len(chunk) < 4:
                   break
               slen = struct.unpack('>L', chunk)[0]
               chunk = self.connection.recv(slen)
               while len(chunk) < slen:
                   chunk = chunk + self.connection.recv(slen - len(chunk))
               obj = self.unPickle(chunk)
               record = logging.makeLogRecord(obj)
               self.handleLogRecord(record)

       def unPickle(self, data):
           return pickle.loads(data)

       def handleLogRecord(self, record):
           # if a name is specified, we use the named logger rather than the one
           # implied by the record.
           if self.server.logname is not None:
               name = self.server.logname
           else:
               name = record.name
           logger = logging.getLogger(name)
           # N.B. EVERY record gets logged. This is because Logger.handle
           # is normally called AFTER logger-level filtering. If you want
           # to do filtering, do it at the client end to save wasting
           # cycles and network bandwidth!
           logger.handle(record)

   class LogRecordSocketReceiver(SocketServer.ThreadingTCPServer):
       """
       Simple TCP socket-based logging receiver suitable for testing.
       """

       allow_reuse_address = 1

       def __init__(self, host='localhost',
                    port=logging.handlers.DEFAULT_TCP_LOGGING_PORT,
                    handler=LogRecordStreamHandler):
           SocketServer.ThreadingTCPServer.__init__(self, (host, port), handler)
           self.abort = 0
           self.timeout = 1
           self.logname = None

       def serve_until_stopped(self):
           import select
           abort = 0
           while not abort:
               rd, wr, ex = select.select([self.socket.fileno()],
                                          [], [],
                                          self.timeout)
               if rd:
                   self.handle_request()
               abort = self.abort

   def main():
       logging.basicConfig(
           format='%(relativeCreated)5d %(name)-15s %(levelname)-8s %(message)s')
       tcpserver = LogRecordSocketReceiver()
       print('About to start TCP server...')
       tcpserver.serve_until_stopped()

   if __name__ == '__main__':
       main()

First run the server, and then the client. On the client side, nothing is
printed on the console; on the server side, you should see something like::

   About to start TCP server...
      59 root            INFO     Jackdaws love my big sphinx of quartz.
      59 myapp.area1     DEBUG    Quick zephyrs blow, vexing daft Jim.
      69 myapp.area1     INFO     How quickly daft jumping zebras vex.
      69 myapp.area2     WARNING  Jail zesty vixen who grabbed pay from quack.
      69 myapp.area2     ERROR    The five boxing wizards jump quickly.

Note that there are some security issues with pickle in some scenarios. If
these affect you, you can use an alternative serialization scheme by overriding
the :meth:`~handlers.SocketHandler.makePickle` method and implementing your
alternative there, as well as adapting the above script to use your alternative
serialization.


.. _context-info:

Adding contextual information to your logging output
----------------------------------------------------

.. currentmodule:: logging

Sometimes you want logging output to contain contextual information in
addition to the parameters passed to the logging call. For example, in a
networked application, it may be desirable to log client-specific information
in the log (e.g. remote client's username, or IP address). Although you could
use the *extra* parameter to achieve this, it's not always convenient to pass
the information in this way. While it might be tempting to create
:class:`Logger` instances on a per-connection basis, this is not a good idea
because these instances are not garbage collected. While this is not a problem
in practice, when the number of :class:`Logger` instances is dependent on the
level of granularity you want to use in logging an application, it could
be hard to manage if the number of :class:`Logger` instances becomes
effectively unbounded.


Using LoggerAdapters to impart contextual information
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

An easy way in which you can pass contextual information to be output along
with logging event information is to use the :class:`LoggerAdapter` class.
This class is designed to look like a :class:`Logger`, so that you can call
:meth:`debug`, :meth:`info`, :meth:`warning`, :meth:`error`,
:meth:`exception`, :meth:`critical` and :meth:`log`. These methods have the
same signatures as their counterparts in :class:`Logger`, so you can use the
two types of instances interchangeably.

When you create an instance of :class:`LoggerAdapter`, you pass it a
:class:`Logger` instance and a dict-like object which contains your contextual
information. When you call one of the logging methods on an instance of
:class:`LoggerAdapter`, it delegates the call to the underlying instance of
:class:`Logger` passed to its constructor, and arranges to pass the contextual
information in the delegated call. Here's a snippet from the code of
:class:`LoggerAdapter`::

    def debug(self, msg, *args, **kwargs):
        """
        Delegate a debug call to the underlying logger, after adding
        contextual information from this adapter instance.
        """
        msg, kwargs = self.process(msg, kwargs)
        self.logger.debug(msg, *args, **kwargs)

The :meth:`~LoggerAdapter.process` method of :class:`LoggerAdapter` is where the
contextual information is added to the logging output. It's passed the message
and keyword arguments of the logging call, and it passes back (potentially)
modified versions of these to use in the call to the underlying logger. The
default implementation of this method leaves the message alone, but inserts
an 'extra' key in the keyword argument whose value is the dict-like object
passed to the constructor. Of course, if you had passed an 'extra' keyword
argument in the call to the adapter, it will be silently overwritten.

The advantage of using 'extra' is that the values in the dict-like object are
merged into the :class:`LogRecord` instance's __dict__, allowing you to use
customized strings with your :class:`Formatter` instances which know about
the keys of the dict-like object. If you need a different method, e.g. if you
want to prepend or append the contextual information to the message string,
you just need to subclass :class:`LoggerAdapter` and override
:meth:`~LoggerAdapter.process` to do what you need. Here is a simple example::

    class CustomAdapter(logging.LoggerAdapter):
        """
        This example adapter expects the passed in dict-like object to have a
        'connid' key, whose value in brackets is prepended to the log message.
        """
        def process(self, msg, kwargs):
            return '[%s] %s' % (self.extra['connid'], msg), kwargs

which you can use like this::

    logger = logging.getLogger(__name__)
    adapter = CustomAdapter(logger, {'connid': some_conn_id})

Then any events that you log to the adapter will have the value of
``some_conn_id`` prepended to the log messages.

Using objects other than dicts to pass contextual information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You don't need to pass an actual dict to a :class:`LoggerAdapter` - you could
pass an instance of a class which implements ``__getitem__`` and ``__iter__`` so
that it looks like a dict to logging. This would be useful if you want to
generate values dynamically (whereas the values in a dict would be constant).


.. _filters-contextual:

Using Filters to impart contextual information
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can also add contextual information to log output using a user-defined
:class:`Filter`. ``Filter`` instances are allowed to modify the ``LogRecords``
passed to them, including adding additional attributes which can then be output
using a suitable format string, or if needed a custom :class:`Formatter`.

For example in a web application, the request being processed (or at least,
the interesting parts of it) can be stored in a threadlocal
(:class:`threading.local`) variable, and then accessed from a ``Filter`` to
add, say, information from the request - say, the remote IP address and remote
user's username - to the ``LogRecord``, using the attribute names 'ip' and
'user' as in the ``LoggerAdapter`` example above. In that case, the same format
string can be used to get similar output to that shown above. Here's an example
script::

    import logging
    from random import choice

    class ContextFilter(logging.Filter):
        """
        This is a filter which injects contextual information into the log.

        Rather than use actual contextual information, we just use random
        data in this demo.
        """

        USERS = ['jim', 'fred', 'sheila']
        IPS = ['123.231.231.123', '127.0.0.1', '192.168.0.1']

        def filter(self, record):

            record.ip = choice(ContextFilter.IPS)
            record.user = choice(ContextFilter.USERS)
            return True

    if __name__ == '__main__':
       levels = (logging.DEBUG, logging.INFO, logging.WARNING, logging.ERROR, logging.CRITICAL)
       logging.basicConfig(level=logging.DEBUG,
                           format='%(asctime)-15s %(name)-5s %(levelname)-8s IP: %(ip)-15s User: %(user)-8s %(message)s')
       a1 = logging.getLogger('a.b.c')
       a2 = logging.getLogger('d.e.f')

       f = ContextFilter()
       a1.addFilter(f)
       a2.addFilter(f)
       a1.debug('A debug message')
       a1.info('An info message with %s', 'some parameters')
       for x in range(10):
           lvl = choice(levels)
           lvlname = logging.getLevelName(lvl)
           a2.log(lvl, 'A message at %s level with %d %s', lvlname, 2, 'parameters')

which, when run, produces something like::

    2010-09-06 22:38:15,292 a.b.c DEBUG    IP: 123.231.231.123 User: fred     A debug message
    2010-09-06 22:38:15,300 a.b.c INFO     IP: 192.168.0.1     User: sheila   An info message with some parameters
    2010-09-06 22:38:15,300 d.e.f CRITICAL IP: 127.0.0.1       User: sheila   A message at CRITICAL level with 2 parameters
    2010-09-06 22:38:15,300 d.e.f ERROR    IP: 127.0.0.1       User: jim      A message at ERROR level with 2 parameters
    2010-09-06 22:38:15,300 d.e.f DEBUG    IP: 127.0.0.1       User: sheila   A message at DEBUG level with 2 parameters
    2010-09-06 22:38:15,300 d.e.f ERROR    IP: 123.231.231.123 User: fred     A message at ERROR level with 2 parameters
    2010-09-06 22:38:15,300 d.e.f CRITICAL IP: 192.168.0.1     User: jim      A message at CRITICAL level with 2 parameters
    2010-09-06 22:38:15,300 d.e.f CRITICAL IP: 127.0.0.1       User: sheila   A message at CRITICAL level with 2 parameters
    2010-09-06 22:38:15,300 d.e.f DEBUG    IP: 192.168.0.1     User: jim      A message at DEBUG level with 2 parameters
    2010-09-06 22:38:15,301 d.e.f ERROR    IP: 127.0.0.1       User: sheila   A message at ERROR level with 2 parameters
    2010-09-06 22:38:15,301 d.e.f DEBUG    IP: 123.231.231.123 User: fred     A message at DEBUG level with 2 parameters
    2010-09-06 22:38:15,301 d.e.f INFO     IP: 123.231.231.123 User: fred     A message at INFO level with 2 parameters


.. _multiple-processes:

Logging to a single file from multiple processes
------------------------------------------------

Although logging is thread-safe, and logging to a single file from multiple
threads in a single process *is* supported, logging to a single file from
*multiple processes* is *not* supported, because there is no standard way to
serialize access to a single file across multiple processes in Python. If you
need to log to a single file from multiple processes, one way of doing this is
to have all the processes log to a :class:`~handlers.SocketHandler`, and have a
separate process which implements a socket server which reads from the socket
and logs to file. (If you prefer, you can dedicate one thread in one of the
existing processes to perform this function.)
:ref:`This section <network-logging>` documents this approach in more detail and
includes a working socket receiver which can be used as a starting point for you
to adapt in your own applications.

If you are using a recent version of Python which includes the
:mod:`multiprocessing` module, you could write your own handler which uses the
:class:`~multiprocessing.Lock` class from this module to serialize access to the
file from your processes. The existing :class:`FileHandler` and subclasses do
not make use of :mod:`multiprocessing` at present, though they may do so in the
future. Note that at present, the :mod:`multiprocessing` module does not provide
working lock functionality on all platforms (see
https://bugs.python.org/issue3770).


Using file rotation
-------------------

.. sectionauthor:: Doug Hellmann, Vinay Sajip (changes)
.. (see <http://blog.doughellmann.com/2007/05/pymotw-logging.html>)

Sometimes you want to let a log file grow to a certain size, then open a new
file and log to that. You may want to keep a certain number of these files, and
when that many files have been created, rotate the files so that the number of
files and the size of the files both remain bounded. For this usage pattern, the
logging package provides a :class:`~handlers.RotatingFileHandler`::

   import glob
   import logging
   import logging.handlers

   LOG_FILENAME = 'logging_rotatingfile_example.out'

   # Set up a specific logger with our desired output level
   my_logger = logging.getLogger('MyLogger')
   my_logger.setLevel(logging.DEBUG)

   # Add the log message handler to the logger
   handler = logging.handlers.RotatingFileHandler(
                 LOG_FILENAME, maxBytes=20, backupCount=5)

   my_logger.addHandler(handler)

   # Log some messages
   for i in range(20):
       my_logger.debug('i = %d' % i)

   # See what files are created
   logfiles = glob.glob('%s*' % LOG_FILENAME)

   for filename in logfiles:
       print(filename)

The result should be 6 separate files, each with part of the log history for the
application::

   logging_rotatingfile_example.out
   logging_rotatingfile_example.out.1
   logging_rotatingfile_example.out.2
   logging_rotatingfile_example.out.3
   logging_rotatingfile_example.out.4
   logging_rotatingfile_example.out.5

The most current file is always :file:`logging_rotatingfile_example.out`,
and each time it reaches the size limit it is renamed with the suffix
``.1``. Each of the existing backup files is renamed to increment the suffix
(``.1`` becomes ``.2``, etc.)  and the ``.6`` file is erased.

Obviously this example sets the log length much too small as an extreme
example.  You would want to set *maxBytes* to an appropriate value.

An example dictionary-based configuration
-----------------------------------------

Below is an example of a logging configuration dictionary - it's taken from
the `documentation on the Django project <https://docs.djangoproject.com/en/1.3/topics/logging/#configuring-logging>`_.
This dictionary is passed to :func:`~config.dictConfig` to put the configuration into effect::

    LOGGING = {
        'version': 1,
        'disable_existing_loggers': True,
        'formatters': {
            'verbose': {
                'format': '%(levelname)s %(asctime)s %(module)s %(process)d %(thread)d %(message)s'
            },
            'simple': {
                'format': '%(levelname)s %(message)s'
            },
        },
        'filters': {
            'special': {
                '()': 'project.logging.SpecialFilter',
                'foo': 'bar',
            }
        },
        'handlers': {
            'null': {
                'level':'DEBUG',
                'class':'django.utils.log.NullHandler',
            },
            'console':{
                'level':'DEBUG',
                'class':'logging.StreamHandler',
                'formatter': 'simple'
            },
            'mail_admins': {
                'level': 'ERROR',
                'class': 'django.utils.log.AdminEmailHandler',
                'filters': ['special']
            }
        },
        'loggers': {
            'django': {
                'handlers':['null'],
                'propagate': True,
                'level':'INFO',
            },
            'django.request': {
                'handlers': ['mail_admins'],
                'level': 'ERROR',
                'propagate': False,
            },
            'myproject.custom': {
                'handlers': ['console', 'mail_admins'],
                'level': 'INFO',
                'filters': ['special']
            }
        }
    }

For more information about this configuration, you can see the `relevant
section <https://docs.djangoproject.com/en/1.6/topics/logging/#configuring-logging>`_
of the Django documentation.

Inserting a BOM into messages sent to a SysLogHandler
-----------------------------------------------------

`RFC 5424 <http://tools.ietf.org/html/rfc5424>`_ requires that a
Unicode message be sent to a syslog daemon as a set of bytes which have the
following structure: an optional pure-ASCII component, followed by a UTF-8 Byte
Order Mark (BOM), followed by Unicode encoded using UTF-8. (See the `relevant
section of the specification <http://tools.ietf.org/html/rfc5424#section-6>`_.)

In Python 2.6 and 2.7, code was added to
:class:`~logging.handlers.SysLogHandler` to insert a BOM into the message, but
unfortunately, it was implemented incorrectly, with the BOM appearing at the
beginning of the message and hence not allowing any pure-ASCII component to
appear before it.

As this behaviour is broken, the incorrect BOM insertion code is being removed
from Python 2.7.4 and later. However, it is not being replaced, and if you
want to produce RFC 5424-compliant messages which include a BOM, an optional
pure-ASCII sequence before it and arbitrary Unicode after it, encoded using
UTF-8, then you need to do the following:

#. Attach a :class:`~logging.Formatter` instance to your
   :class:`~logging.handlers.SysLogHandler` instance, with a format string
   such as::

      u'ASCII section\ufeffUnicode section'

   The Unicode code point ``u'\ufeff'``, when encoded using UTF-8, will be
   encoded as a UTF-8 BOM -- the byte-string ``'\xef\xbb\xbf'``.

#. Replace the ASCII section with whatever placeholders you like, but make sure
   that the data that appears in there after substitution is always ASCII (that
   way, it will remain unchanged after UTF-8 encoding).

#. Replace the Unicode section with whatever placeholders you like; if the data
   which appears there after substitution contains characters outside the ASCII
   range, that's fine -- it will be encoded using UTF-8.

If the formatted message is Unicode, it *will* be encoded using UTF-8 encoding
by ``SysLogHandler``. If you follow the above rules, you should be able to
produce RFC 5424-compliant messages. If you don't, logging may not complain,
but your messages will not be RFC 5424-compliant, and your syslog daemon may
complain.


Implementing structured logging
-------------------------------

Although most logging messages are intended for reading by humans, and thus not
readily machine-parseable, there might be cirumstances where you want to output
messages in a structured format which *is* capable of being parsed by a program
(without needing complex regular expressions to parse the log message). This is
straightforward to achieve using the logging package. There are a number of
ways in which this could be achieved, but the following is a simple approach
which uses JSON to serialise the event in a machine-parseable manner::

    import json
    import logging

    class StructuredMessage(object):
        def __init__(self, message, **kwargs):
            self.message = message
            self.kwargs = kwargs

        def __str__(self):
            return '%s >>> %s' % (self.message, json.dumps(self.kwargs))

    _ = StructuredMessage   # optional, to improve readability

    logging.basicConfig(level=logging.INFO, format='%(message)s')
    logging.info(_('message 1', foo='bar', bar='baz', num=123, fnum=123.456))

If the above script is run, it prints::

    message 1 >>> {"fnum": 123.456, "num": 123, "bar": "baz", "foo": "bar"}

Note that the order of items might be different according to the version of
Python used.

If you need more specialised processing, you can use a custom JSON encoder,
as in the following complete example::

    from __future__ import unicode_literals

    import json
    import logging

    # This next bit is to ensure the script runs unchanged on 2.x and 3.x
    try:
        unicode
    except NameError:
        unicode = str

    class Encoder(json.JSONEncoder):
        def default(self, o):
            if isinstance(o, set):
                return tuple(o)
            elif isinstance(o, unicode):
                return o.encode('unicode_escape').decode('ascii')
            return super(Encoder, self).default(o)

    class StructuredMessage(object):
        def __init__(self, message, **kwargs):
            self.message = message
            self.kwargs = kwargs

        def __str__(self):
            s = Encoder().encode(self.kwargs)
            return '%s >>> %s' % (self.message, s)

    _ = StructuredMessage   # optional, to improve readability

    def main():
        logging.basicConfig(level=logging.INFO, format='%(message)s')
        logging.info(_('message 1', set_value=set([1, 2, 3]), snowman='\u2603'))

    if __name__ == '__main__':
        main()

When the above script is run, it prints::

    message 1 >>> {"snowman": "\u2603", "set_value": [1, 2, 3]}

Note that the order of items might be different according to the version of
Python used.


.. _custom-handlers:

.. currentmodule:: logging.config

Customizing handlers with :func:`dictConfig`
--------------------------------------------

There are times when you want to customize logging handlers in particular ways,
and if you use :func:`dictConfig` you may be able to do this without
subclassing. As an example, consider that you may want to set the ownership of a
log file. On POSIX, this is easily done using :func:`os.chown`, but the file
handlers in the stdlib don't offer built-in support. You can customize handler
creation using a plain function such as::

    def owned_file_handler(filename, mode='a', encoding=None, owner=None):
        if owner:
            import os, pwd, grp
            # convert user and group names to uid and gid
            uid = pwd.getpwnam(owner[0]).pw_uid
            gid = grp.getgrnam(owner[1]).gr_gid
            owner = (uid, gid)
            if not os.path.exists(filename):
                open(filename, 'a').close()
            os.chown(filename, *owner)
        return logging.FileHandler(filename, mode, encoding)

You can then specify, in a logging configuration passed to :func:`dictConfig`,
that a logging handler be created by calling this function::

    LOGGING = {
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'default': {
                'format': '%(asctime)s %(levelname)s %(name)s %(message)s'
            },
        },
        'handlers': {
            'file':{
                # The values below are popped from this dictionary and
                # used to create the handler, set the handler's level and
                # its formatter.
                '()': owned_file_handler,
                'level':'DEBUG',
                'formatter': 'default',
                # The values below are passed to the handler creator callable
                # as keyword arguments.
                'owner': ['pulse', 'pulse'],
                'filename': 'chowntest.log',
                'mode': 'w',
                'encoding': 'utf-8',
            },
        },
        'root': {
            'handlers': ['file'],
            'level': 'DEBUG',
        },
    }

In this example I am setting the ownership using the ``pulse`` user and group,
just for the purposes of illustration. Putting it together into a working
script, ``chowntest.py``::

    import logging, logging.config, os, shutil

    def owned_file_handler(filename, mode='a', encoding=None, owner=None):
        if owner:
            if not os.path.exists(filename):
                open(filename, 'a').close()
            shutil.chown(filename, *owner)
        return logging.FileHandler(filename, mode, encoding)

    LOGGING = {
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'default': {
                'format': '%(asctime)s %(levelname)s %(name)s %(message)s'
            },
        },
        'handlers': {
            'file':{
                # The values below are popped from this dictionary and
                # used to create the handler, set the handler's level and
                # its formatter.
                '()': owned_file_handler,
                'level':'DEBUG',
                'formatter': 'default',
                # The values below are passed to the handler creator callable
                # as keyword arguments.
                'owner': ['pulse', 'pulse'],
                'filename': 'chowntest.log',
                'mode': 'w',
                'encoding': 'utf-8',
            },
        },
        'root': {
            'handlers': ['file'],
            'level': 'DEBUG',
        },
    }

    logging.config.dictConfig(LOGGING)
    logger = logging.getLogger('mylogger')
    logger.debug('A debug message')

To run this, you will probably need to run as ``root``::

    $ sudo python3.3 chowntest.py
    $ cat chowntest.log
    2013-11-05 09:34:51,128 DEBUG mylogger A debug message
    $ ls -l chowntest.log
    -rw-r--r-- 1 pulse pulse 55 2013-11-05 09:34 chowntest.log

Note that this example uses Python 3.3 because that's where :func:`shutil.chown`
makes an appearance. This approach should work with any Python version that
supports :func:`dictConfig` - namely, Python 2.7, 3.2 or later. With pre-3.3
versions, you would need to implement the actual ownership change using e.g.
:func:`os.chown`.

In practice, the handler-creating function may be in a utility module somewhere
in your project. Instead of the line in the configuration::

    '()': owned_file_handler,

you could use e.g.::

    '()': 'ext://project.util.owned_file_handler',

where ``project.util`` can be replaced with the actual name of the package
where the function resides. In the above working script, using
``'ext://__main__.owned_file_handler'`` should work. Here, the actual callable
is resolved by :func:`dictConfig` from the ``ext://`` specification.

This example hopefully also points the way to how you could implement other
types of file change - e.g. setting specific POSIX permission bits - in the
same way, using :func:`os.chmod`.

Of course, the approach could also be extended to types of handler other than a
:class:`~logging.FileHandler` - for example, one of the rotating file handlers,
or a different type of handler altogether.


.. _filters-dictconfig:

Configuring filters with :func:`dictConfig`
-------------------------------------------

You *can* configure filters using :func:`~logging.config.dictConfig`, though it
might not be obvious at first glance how to do it (hence this recipe). Since
:class:`~logging.Filter` is the only filter class included in the standard
library, and it is unlikely to cater to many requirements (it's only there as a
base class), you will typically need to define your own :class:`~logging.Filter`
subclass with an overridden :meth:`~logging.Filter.filter` method. To do this,
specify the ``()`` key in the configuration dictionary for the filter,
specifying a callable which will be used to create the filter (a class is the
most obvious, but you can provide any callable which returns a
:class:`~logging.Filter` instance). Here is a complete example::

    import logging
    import logging.config
    import sys

    class MyFilter(logging.Filter):
        def __init__(self, param=None):
            self.param = param

        def filter(self, record):
            if self.param is None:
                allow = True
            else:
                allow = self.param not in record.msg
            if allow:
                record.msg = 'changed: ' + record.msg
            return allow

    LOGGING = {
        'version': 1,
        'filters': {
            'myfilter': {
                '()': MyFilter,
                'param': 'noshow',
            }
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'filters': ['myfilter']
            }
        },
        'root': {
            'level': 'DEBUG',
            'handlers': ['console']
        },
    }

    if __name__ == '__main__':
        logging.config.dictConfig(LOGGING)
        logging.debug('hello')
        logging.debug('hello - noshow')

This example shows how you can pass configuration data to the callable which
constructs the instance, in the form of keyword parameters. When run, the above
script will print::

    changed: hello

which shows that the filter is working as configured.

A couple of extra points to note:

* If you can't refer to the callable directly in the configuration (e.g. if it
  lives in a different module, and you can't import it directly where the
  configuration dictionary is), you can use the form ``ext://...`` as described
  in :ref:`logging-config-dict-externalobj`. For example, you could have used
  the text ``'ext://__main__.MyFilter'`` instead of ``MyFilter`` in the above
  example.

* As well as for filters, this technique can also be used to configure custom
  handlers and formatters. See :ref:`logging-config-dict-userdef` for more
  information on how logging supports using user-defined objects in its
  configuration, and see the other cookbook recipe :ref:`custom-handlers` above.


.. _custom-format-exception:

Customized exception formatting
-------------------------------

There might be times when you want to do customized exception formatting - for
argument's sake, let's say you want exactly one line per logged event, even
when exception information is present. You can do this with a custom formatter
class, as shown in the following example::

    import logging

    class OneLineExceptionFormatter(logging.Formatter):
        def formatException(self, exc_info):
            """
            Format an exception so that it prints on a single line.
            """
            result = super(OneLineExceptionFormatter, self).formatException(exc_info)
            return repr(result) # or format into one line however you want to

        def format(self, record):
            s = super(OneLineExceptionFormatter, self).format(record)
            if record.exc_text:
                s = s.replace('\n', '') + '|'
            return s

    def configure_logging():
        fh = logging.FileHandler('output.txt', 'w')
        f = OneLineExceptionFormatter('%(asctime)s|%(levelname)s|%(message)s|',
                                      '%d/%m/%Y %H:%M:%S')
        fh.setFormatter(f)
        root = logging.getLogger()
        root.setLevel(logging.DEBUG)
        root.addHandler(fh)

    def main():
        configure_logging()
        logging.info('Sample message')
        try:
            x = 1 / 0
        except ZeroDivisionError as e:
            logging.exception('ZeroDivisionError: %s', e)

    if __name__ == '__main__':
        main()

When run, this produces a file with exactly two lines::

    28/01/2015 07:21:23|INFO|Sample message|
    28/01/2015 07:21:23|ERROR|ZeroDivisionError: integer division or modulo by zero|'Traceback (most recent call last):\n  File "logtest7.py", line 30, in main\n    x = 1 / 0\nZeroDivisionError: integer division or modulo by zero'|

While the above treatment is simplistic, it points the way to how exception
information can be formatted to your liking. The :mod:`traceback` module may be
helpful for more specialized needs.

.. _spoken-messages:

Speaking logging messages
-------------------------

There might be situations when it is desirable to have logging messages rendered
in an audible rather than a visible format. This is easy to do if you have text-
to-speech (TTS) functionality available in your system, even if it doesn't have
a Python binding. Most TTS systems have a command line program you can run, and
this can be invoked from a handler using :mod:`subprocess`. It's assumed here
that TTS command line programs won't expect to interact with users or take a
long time to complete, and that the frequency of logged messages will be not so
high as to swamp the user with messages, and that it's acceptable to have the
messages spoken one at a time rather than concurrently, The example implementation
below waits for one message to be spoken before the next is processed, and this
might cause other handlers to be kept waiting. Here is a short example showing
the approach, which assumes that the ``espeak`` TTS package is available::

    import logging
    import subprocess
    import sys

    class TTSHandler(logging.Handler):
        def emit(self, record):
            msg = self.format(record)
            # Speak slowly in a female English voice
            cmd = ['espeak', '-s150', '-ven+f3', msg]
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT)
            # wait for the program to finish
            p.communicate()

    def configure_logging():
        h = TTSHandler()
        root = logging.getLogger()
        root.addHandler(h)
        # the default formatter just returns the message
        root.setLevel(logging.DEBUG)

    def main():
        logging.info('Hello')
        logging.debug('Goodbye')

    if __name__ == '__main__':
        configure_logging()
        sys.exit(main())

When run, this script should say "Hello" and then "Goodbye" in a female voice.

The above approach can, of course, be adapted to other TTS systems and even
other systems altogether which can process messages via external programs run
from a command line.

.. _buffered-logging:

Buffering logging messages and outputting them conditionally
------------------------------------------------------------

There might be situations where you want to log messages in a temporary area
and only output them if a certain condition occurs. For example, you may want to
start logging debug events in a function, and if the function completes without
errors, you don't want to clutter the log with the collected debug information,
but if there is an error, you want all the debug information to be output as well
as the error.

Here is an example which shows how you could do this using a decorator for your
functions where you want logging to behave this way. It makes use of the
:class:`logging.handlers.MemoryHandler`, which allows buffering of logged events
until some condition occurs, at which point the buffered events are ``flushed``
- passed to another handler (the ``target`` handler) for processing. By default,
the ``MemoryHandler`` flushed when its buffer gets filled up or an event whose
level is greater than or equal to a specified threshold is seen. You can use this
recipe with a more specialised subclass of ``MemoryHandler`` if you want custom
flushing behavior.

The example script has a simple function, ``foo``, which just cycles through
all the logging levels, writing to ``sys.stderr`` to say what level it's about
to log at, and then actually logging a message that that level. You can pass a
parameter to ``foo`` which, if true, will log at ERROR and CRITICAL levels -
otherwise, it only logs at DEBUG, INFO and WARNING levels.

The script just arranges to decorate ``foo`` with a decorator which will do the
conditional logging that's required. The decorator takes a logger as a parameter
and attaches a memory handler for the duration of the call to the decorated
function. The decorator can be additionally parameterised using a target handler,
a level at which flushing should occur, and a capacity for the buffer. These
default to a :class:`~logging.StreamHandler` which writes to ``sys.stderr``,
``logging.ERROR`` and ``100`` respectively.

Here's the script::

    import logging
    from logging.handlers import MemoryHandler
    import sys

    logger = logging.getLogger(__name__)
    logger.addHandler(logging.NullHandler())

    def log_if_errors(logger, target_handler=None, flush_level=None, capacity=None):
        if target_handler is None:
            target_handler = logging.StreamHandler()
        if flush_level is None:
            flush_level = logging.ERROR
        if capacity is None:
            capacity = 100
        handler = MemoryHandler(capacity, flushLevel=flush_level, target=target_handler)

        def decorator(fn):
            def wrapper(*args, **kwargs):
                logger.addHandler(handler)
                try:
                    return fn(*args, **kwargs)
                except Exception:
                    logger.exception('call failed')
                    raise
                finally:
                    super(MemoryHandler, handler).flush()
                    logger.removeHandler(handler)
            return wrapper

        return decorator

    def write_line(s):
        sys.stderr.write('%s\n' % s)

    def foo(fail=False):
        write_line('about to log at DEBUG ...')
        logger.debug('Actually logged at DEBUG')
        write_line('about to log at INFO ...')
        logger.info('Actually logged at INFO')
        write_line('about to log at WARNING ...')
        logger.warning('Actually logged at WARNING')
        if fail:
            write_line('about to log at ERROR ...')
            logger.error('Actually logged at ERROR')
            write_line('about to log at CRITICAL ...')
            logger.critical('Actually logged at CRITICAL')
        return fail

    decorated_foo = log_if_errors(logger)(foo)

    if __name__ == '__main__':
        logger.setLevel(logging.DEBUG)
        write_line('Calling undecorated foo with False')
        assert not foo(False)
        write_line('Calling undecorated foo with True')
        assert foo(True)
        write_line('Calling decorated foo with False')
        assert not decorated_foo(False)
        write_line('Calling decorated foo with True')
        assert decorated_foo(True)

When this script is run, the following output should be observed::

    Calling undecorated foo with False
    about to log at DEBUG ...
    about to log at INFO ...
    about to log at WARNING ...
    Calling undecorated foo with True
    about to log at DEBUG ...
    about to log at INFO ...
    about to log at WARNING ...
    about to log at ERROR ...
    about to log at CRITICAL ...
    Calling decorated foo with False
    about to log at DEBUG ...
    about to log at INFO ...
    about to log at WARNING ...
    Calling decorated foo with True
    about to log at DEBUG ...
    about to log at INFO ...
    about to log at WARNING ...
    about to log at ERROR ...
    Actually logged at DEBUG
    Actually logged at INFO
    Actually logged at WARNING
    Actually logged at ERROR
    about to log at CRITICAL ...
    Actually logged at CRITICAL

As you can see, actual logging output only occurs when an event is logged whose
severity is ERROR or greater, but in that case, any previous events at lower
severities are also logged.

You can of course use the conventional means of decoration::

    @log_if_errors(logger)
    def foo(fail=False):
        ...


.. _utc-formatting:

Formatting times using UTC (GMT) via configuration
--------------------------------------------------

Sometimes you want to format times using UTC, which can be done using a class
such as `UTCFormatter`, shown below::

    import logging
    import time

    class UTCFormatter(logging.Formatter):
        converter = time.gmtime

and you can then use the ``UTCFormatter`` in your code instead of
:class:`~logging.Formatter`. If you want to do that via configuration, you can
use the :func:`~logging.config.dictConfig` API with an approach illustrated by
the following complete example::

    import logging
    import logging.config
    import time

    class UTCFormatter(logging.Formatter):
        converter = time.gmtime

    LOGGING = {
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'utc': {
                '()': UTCFormatter,
                'format': '%(asctime)s %(message)s',
            },
            'local': {
                'format': '%(asctime)s %(message)s',
            }
        },
        'handlers': {
            'console1': {
                'class': 'logging.StreamHandler',
                'formatter': 'utc',
            },
            'console2': {
                'class': 'logging.StreamHandler',
                'formatter': 'local',
            },
        },
        'root': {
            'handlers': ['console1', 'console2'],
       }
    }

    if __name__ == '__main__':
        logging.config.dictConfig(LOGGING)
        logging.warning('The local time is %s', time.asctime())

When this script is run, it should print something like::

    2015-10-17 12:53:29,501 The local time is Sat Oct 17 13:53:29 2015
    2015-10-17 13:53:29,501 The local time is Sat Oct 17 13:53:29 2015

showing how the time is formatted both as local time and UTC, one for each
handler.
