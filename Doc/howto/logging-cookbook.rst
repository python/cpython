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

Loggers are plain Python objects.  The :func:`addHandler` method has no minimum
or maximum quota for the number of handlers you may add.  Sometimes it will be
beneficial for an application to log all messages of all severities to a text
file while simultaneously logging errors or above to the console.  To set this
up, simply configure the appropriate handlers.  The logging calls in the
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

At the receiving end, you can set up a receiver using the :mod:`socketserver`
module. Here is a basic working example::

   import pickle
   import logging
   import logging.handlers
   import socketserver
   import struct


   class LogRecordStreamHandler(socketserver.StreamRequestHandler):
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

   class LogRecordSocketReceiver(socketserver.ThreadingTCPServer):
       """
       Simple TCP socket-based logging receiver suitable for testing.
       """

       allow_reuse_address = 1

       def __init__(self, host='localhost',
                    port=logging.handlers.DEFAULT_TCP_LOGGING_PORT,
                    handler=LogRecordStreamHandler):
           socketserver.ThreadingTCPServer.__init__(self, (host, port), handler)
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
the :meth:`makePickle` method and implementing your alternative there, as
well as adapting the above script to use your alternative serialization.


.. _context-info:

Adding contextual information to your logging output
----------------------------------------------------

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

The :meth:`process` method of :class:`LoggerAdapter` is where the contextual
information is added to the logging output. It's passed the message and
keyword arguments of the logging call, and it passes back (potentially)
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
you just need to subclass :class:`LoggerAdapter` and override :meth:`process`
to do what you need. Here's an example script which uses this class, which
also illustrates what dict-like behaviour is needed from an arbitrary
'dict-like' object for use in the constructor::

   import logging

   class ConnInfo:
       """
       An example class which shows how an arbitrary class can be used as
       the 'extra' context information repository passed to a LoggerAdapter.
       """

       def __getitem__(self, name):
           """
           To allow this instance to look like a dict.
           """
           from random import choice
           if name == 'ip':
               result = choice(['127.0.0.1', '192.168.0.1'])
           elif name == 'user':
               result = choice(['jim', 'fred', 'sheila'])
           else:
               result = self.__dict__.get(name, '?')
           return result

       def __iter__(self):
           """
           To allow iteration over keys, which will be merged into
           the LogRecord dict before formatting and output.
           """
           keys = ['ip', 'user']
           keys.extend(self.__dict__.keys())
           return keys.__iter__()

   if __name__ == '__main__':
       from random import choice
       levels = (logging.DEBUG, logging.INFO, logging.WARNING, logging.ERROR, logging.CRITICAL)
       a1 = logging.LoggerAdapter(logging.getLogger('a.b.c'),
                                  { 'ip' : '123.231.231.123', 'user' : 'sheila' })
       logging.basicConfig(level=logging.DEBUG,
                           format='%(asctime)-15s %(name)-5s %(levelname)-8s IP: %(ip)-15s User: %(user)-8s %(message)s')
       a1.debug('A debug message')
       a1.info('An info message with %s', 'some parameters')
       a2 = logging.LoggerAdapter(logging.getLogger('d.e.f'), ConnInfo())
       for x in range(10):
           lvl = choice(levels)
           lvlname = logging.getLevelName(lvl)
           a2.log(lvl, 'A message at %s level with %d %s', lvlname, 2, 'parameters')

When this script is run, the output should look something like this::

   2008-01-18 14:49:54,023 a.b.c DEBUG    IP: 123.231.231.123 User: sheila   A debug message
   2008-01-18 14:49:54,023 a.b.c INFO     IP: 123.231.231.123 User: sheila   An info message with some parameters
   2008-01-18 14:49:54,023 d.e.f CRITICAL IP: 192.168.0.1     User: jim      A message at CRITICAL level with 2 parameters
   2008-01-18 14:49:54,033 d.e.f INFO     IP: 192.168.0.1     User: jim      A message at INFO level with 2 parameters
   2008-01-18 14:49:54,033 d.e.f WARNING  IP: 192.168.0.1     User: sheila   A message at WARNING level with 2 parameters
   2008-01-18 14:49:54,033 d.e.f ERROR    IP: 127.0.0.1       User: fred     A message at ERROR level with 2 parameters
   2008-01-18 14:49:54,033 d.e.f ERROR    IP: 127.0.0.1       User: sheila   A message at ERROR level with 2 parameters
   2008-01-18 14:49:54,033 d.e.f WARNING  IP: 192.168.0.1     User: sheila   A message at WARNING level with 2 parameters
   2008-01-18 14:49:54,033 d.e.f WARNING  IP: 192.168.0.1     User: jim      A message at WARNING level with 2 parameters
   2008-01-18 14:49:54,033 d.e.f INFO     IP: 192.168.0.1     User: fred     A message at INFO level with 2 parameters
   2008-01-18 14:49:54,033 d.e.f WARNING  IP: 192.168.0.1     User: sheila   A message at WARNING level with 2 parameters
   2008-01-18 14:49:54,033 d.e.f WARNING  IP: 127.0.0.1       User: jim      A message at WARNING level with 2 parameters


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
to have all the processes log to a :class:`SocketHandler`, and have a separate
process which implements a socket server which reads from the socket and logs
to file. (If you prefer, you can dedicate one thread in one of the existing
processes to perform this function.) :ref:`This section <network-logging>`
documents this approach in more detail and includes a working socket receiver
which can be used as a starting point for you to adapt in your own
applications.

If you are using a recent version of Python which includes the
:mod:`multiprocessing` module, you could write your own handler which uses the
:class:`Lock` class from this module to serialize access to the file from
your processes. The existing :class:`FileHandler` and subclasses do not make
use of :mod:`multiprocessing` at present, though they may do so in the future.
Note that at present, the :mod:`multiprocessing` module does not provide
working lock functionality on all platforms (see
http://bugs.python.org/issue3770).

.. currentmodule:: logging.handlers


Using file rotation
-------------------

.. sectionauthor:: Doug Hellmann, Vinay Sajip (changes)
.. (see <http://blog.doughellmann.com/2007/05/pymotw-logging.html>)

Sometimes you want to let a log file grow to a certain size, then open a new
file and log to that. You may want to keep a certain number of these files, and
when that many files have been created, rotate the files so that the number of
files and the size of the files both remain bounded. For this usage pattern, the
logging package provides a :class:`RotatingFileHandler`::

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
This dictionary is passed to :func:`~logging.config.dictConfig` to put the configuration into effect::

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
section <https://docs.djangoproject.com/en/1.3/topics/logging/#configuring-logging>`_
of the Django documentation.
