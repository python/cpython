:mod:`logging.config` --- Logging configuration
===============================================

.. module:: logging.config
   :synopsis: Configuration of the logging module.


.. moduleauthor:: Vinay Sajip <vinay_sajip@red-dove.com>
.. sectionauthor:: Vinay Sajip <vinay_sajip@red-dove.com>

.. sidebar:: Important

   This page contains only reference information. For tutorials,
   please see

   * :ref:`Basic Tutorial <logging-basic-tutorial>`
   * :ref:`Advanced Tutorial <logging-advanced-tutorial>`
   * :ref:`Logging Cookbook <logging-cookbook>`

This section describes the API for configuring the logging module.

.. _logging-config-api:

Configuration functions
^^^^^^^^^^^^^^^^^^^^^^^

The following functions configure the logging module. They are located in the
:mod:`logging.config` module.  Their use is optional --- you can configure the
logging module using these functions or by making calls to the main API (defined
in :mod:`logging` itself) and defining handlers which are declared either in
:mod:`logging` or :mod:`logging.handlers`.

.. function:: dictConfig(config)

    Takes the logging configuration from a dictionary.  The contents of
    this dictionary are described in :ref:`logging-config-dictschema`
    below.

    If an error is encountered during configuration, this function will
    raise a :exc:`ValueError`, :exc:`TypeError`, :exc:`AttributeError`
    or :exc:`ImportError` with a suitably descriptive message.  The
    following is a (possibly incomplete) list of conditions which will
    raise an error:

    * A ``level`` which is not a string or which is a string not
      corresponding to an actual logging level.
    * A ``propagate`` value which is not a boolean.
    * An id which does not have a corresponding destination.
    * A non-existent handler id found during an incremental call.
    * An invalid logger name.
    * Inability to resolve to an internal or external object.

    Parsing is performed by the :class:`DictConfigurator` class, whose
    constructor is passed the dictionary used for configuration, and
    has a :meth:`configure` method.  The :mod:`logging.config` module
    has a callable attribute :attr:`dictConfigClass`
    which is initially set to :class:`DictConfigurator`.
    You can replace the value of :attr:`dictConfigClass` with a
    suitable implementation of your own.

    :func:`dictConfig` calls :attr:`dictConfigClass` passing
    the specified dictionary, and then calls the :meth:`configure` method on
    the returned object to put the configuration into effect::

          def dictConfig(config):
              dictConfigClass(config).configure()

    For example, a subclass of :class:`DictConfigurator` could call
    ``DictConfigurator.__init__()`` in its own :meth:`__init__()`, then
    set up custom prefixes which would be usable in the subsequent
    :meth:`configure` call. :attr:`dictConfigClass` would be bound to
    this new subclass, and then :func:`dictConfig` could be called exactly as
    in the default, uncustomized state.

   .. versionadded:: 2.7

.. function:: fileConfig(fname[, defaults])

   Reads the logging configuration from a :mod:`configparser`\-format file named
   *fname*. This function can be called several times from an application,
   allowing an end user to select from various pre-canned
   configurations (if the developer provides a mechanism to present the choices
   and load the chosen configuration). Defaults to be passed to the ConfigParser
   can be specified in the *defaults* argument.


.. function:: listen(port=DEFAULT_LOGGING_CONFIG_PORT)

   Starts up a socket server on the specified port, and listens for new
   configurations. If no port is specified, the module's default
   :const:`DEFAULT_LOGGING_CONFIG_PORT` is used. Logging configurations will be
   sent as a file suitable for processing by :func:`fileConfig`. Returns a
   :class:`Thread` instance on which you can call :meth:`start` to start the
   server, and which you can :meth:`join` when appropriate. To stop the server,
   call :func:`stopListening`.

   To send a configuration to the socket, read in the configuration file and
   send it to the socket as a string of bytes preceded by a four-byte length
   string packed in binary using ``struct.pack('>L', n)``.


.. function:: stopListening()

   Stops the listening server which was created with a call to :func:`listen`.
   This is typically called before calling :meth:`join` on the return value from
   :func:`listen`.


.. _logging-config-dictschema:

Configuration dictionary schema
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Describing a logging configuration requires listing the various
objects to create and the connections between them; for example, you
may create a handler named 'console' and then say that the logger
named 'startup' will send its messages to the 'console' handler.
These objects aren't limited to those provided by the :mod:`logging`
module because you might write your own formatter or handler class.
The parameters to these classes may also need to include external
objects such as ``sys.stderr``.  The syntax for describing these
objects and connections is defined in :ref:`logging-config-dict-connections`
below.

Dictionary Schema Details
"""""""""""""""""""""""""

The dictionary passed to :func:`dictConfig` must contain the following
keys:

* *version* - to be set to an integer value representing the schema
  version.  The only valid value at present is 1, but having this key
  allows the schema to evolve while still preserving backwards
  compatibility.

All other keys are optional, but if present they will be interpreted
as described below.  In all cases below where a 'configuring dict' is
mentioned, it will be checked for the special ``'()'`` key to see if a
custom instantiation is required.  If so, the mechanism described in
:ref:`logging-config-dict-userdef` below is used to create an instance;
otherwise, the context is used to determine what to instantiate.

* *formatters* - the corresponding value will be a dict in which each
  key is a formatter id and each value is a dict describing how to
  configure the corresponding Formatter instance.

  The configuring dict is searched for keys ``format`` and ``datefmt``
  (with defaults of ``None``) and these are used to construct a
  :class:`logging.Formatter` instance.

* *filters* - the corresponding value will be a dict in which each key
  is a filter id and each value is a dict describing how to configure
  the corresponding Filter instance.

  The configuring dict is searched for the key ``name`` (defaulting to the
  empty string) and this is used to construct a :class:`logging.Filter`
  instance.

* *handlers* - the corresponding value will be a dict in which each
  key is a handler id and each value is a dict describing how to
  configure the corresponding Handler instance.

  The configuring dict is searched for the following keys:

  * ``class`` (mandatory).  This is the fully qualified name of the
    handler class.

  * ``level`` (optional).  The level of the handler.

  * ``formatter`` (optional).  The id of the formatter for this
    handler.

  * ``filters`` (optional).  A list of ids of the filters for this
    handler.

  All *other* keys are passed through as keyword arguments to the
  handler's constructor.  For example, given the snippet::

      handlers:
        console:
          class : logging.StreamHandler
          formatter: brief
          level   : INFO
          filters: [allow_foo]
          stream  : ext://sys.stdout
        file:
          class : logging.handlers.RotatingFileHandler
          formatter: precise
          filename: logconfig.log
          maxBytes: 1024
          backupCount: 3

  the handler with id ``console`` is instantiated as a
  :class:`logging.StreamHandler`, using ``sys.stdout`` as the underlying
  stream.  The handler with id ``file`` is instantiated as a
  :class:`logging.handlers.RotatingFileHandler` with the keyword arguments
  ``filename='logconfig.log', maxBytes=1024, backupCount=3``.

* *loggers* - the corresponding value will be a dict in which each key
  is a logger name and each value is a dict describing how to
  configure the corresponding Logger instance.

  The configuring dict is searched for the following keys:

  * ``level`` (optional).  The level of the logger.

  * ``propagate`` (optional).  The propagation setting of the logger.

  * ``filters`` (optional).  A list of ids of the filters for this
    logger.

  * ``handlers`` (optional).  A list of ids of the handlers for this
    logger.

  The specified loggers will be configured according to the level,
  propagation, filters and handlers specified.

* *root* - this will be the configuration for the root logger.
  Processing of the configuration will be as for any logger, except
  that the ``propagate`` setting will not be applicable.

* *incremental* - whether the configuration is to be interpreted as
  incremental to the existing configuration.  This value defaults to
  ``False``, which means that the specified configuration replaces the
  existing configuration with the same semantics as used by the
  existing :func:`fileConfig` API.

  If the specified value is ``True``, the configuration is processed
  as described in the section on :ref:`logging-config-dict-incremental`.

* *disable_existing_loggers* - whether any existing loggers are to be
  disabled. This setting mirrors the parameter of the same name in
  :func:`fileConfig`. If absent, this parameter defaults to ``True``.
  This value is ignored if *incremental* is ``True``.

.. _logging-config-dict-incremental:

Incremental Configuration
"""""""""""""""""""""""""

It is difficult to provide complete flexibility for incremental
configuration.  For example, because objects such as filters
and formatters are anonymous, once a configuration is set up, it is
not possible to refer to such anonymous objects when augmenting a
configuration.

Furthermore, there is not a compelling case for arbitrarily altering
the object graph of loggers, handlers, filters, formatters at
run-time, once a configuration is set up; the verbosity of loggers and
handlers can be controlled just by setting levels (and, in the case of
loggers, propagation flags).  Changing the object graph arbitrarily in
a safe way is problematic in a multi-threaded environment; while not
impossible, the benefits are not worth the complexity it adds to the
implementation.

Thus, when the ``incremental`` key of a configuration dict is present
and is ``True``, the system will completely ignore any ``formatters`` and
``filters`` entries, and process only the ``level``
settings in the ``handlers`` entries, and the ``level`` and
``propagate`` settings in the ``loggers`` and ``root`` entries.

Using a value in the configuration dict lets configurations to be sent
over the wire as pickled dicts to a socket listener. Thus, the logging
verbosity of a long-running application can be altered over time with
no need to stop and restart the application.

.. _logging-config-dict-connections:

Object connections
""""""""""""""""""

The schema describes a set of logging objects - loggers,
handlers, formatters, filters - which are connected to each other in
an object graph.  Thus, the schema needs to represent connections
between the objects.  For example, say that, once configured, a
particular logger has attached to it a particular handler.  For the
purposes of this discussion, we can say that the logger represents the
source, and the handler the destination, of a connection between the
two.  Of course in the configured objects this is represented by the
logger holding a reference to the handler.  In the configuration dict,
this is done by giving each destination object an id which identifies
it unambiguously, and then using the id in the source object's
configuration to indicate that a connection exists between the source
and the destination object with that id.

So, for example, consider the following YAML snippet::

    formatters:
      brief:
        # configuration for formatter with id 'brief' goes here
      precise:
        # configuration for formatter with id 'precise' goes here
    handlers:
      h1: #This is an id
       # configuration of handler with id 'h1' goes here
       formatter: brief
      h2: #This is another id
       # configuration of handler with id 'h2' goes here
       formatter: precise
    loggers:
      foo.bar.baz:
        # other configuration for logger 'foo.bar.baz'
        handlers: [h1, h2]

(Note: YAML used here because it's a little more readable than the
equivalent Python source form for the dictionary.)

The ids for loggers are the logger names which would be used
programmatically to obtain a reference to those loggers, e.g.
``foo.bar.baz``.  The ids for Formatters and Filters can be any string
value (such as ``brief``, ``precise`` above) and they are transient,
in that they are only meaningful for processing the configuration
dictionary and used to determine connections between objects, and are
not persisted anywhere when the configuration call is complete.

The above snippet indicates that logger named ``foo.bar.baz`` should
have two handlers attached to it, which are described by the handler
ids ``h1`` and ``h2``. The formatter for ``h1`` is that described by id
``brief``, and the formatter for ``h2`` is that described by id
``precise``.


.. _logging-config-dict-userdef:

User-defined objects
""""""""""""""""""""

The schema supports user-defined objects for handlers, filters and
formatters.  (Loggers do not need to have different types for
different instances, so there is no support in this configuration
schema for user-defined logger classes.)

Objects to be configured are described by dictionaries
which detail their configuration.  In some places, the logging system
will be able to infer from the context how an object is to be
instantiated, but when a user-defined object is to be instantiated,
the system will not know how to do this.  In order to provide complete
flexibility for user-defined object instantiation, the user needs
to provide a 'factory' - a callable which is called with a
configuration dictionary and which returns the instantiated object.
This is signalled by an absolute import path to the factory being
made available under the special key ``'()'``.  Here's a concrete
example::

    formatters:
      brief:
        format: '%(message)s'
      default:
        format: '%(asctime)s %(levelname)-8s %(name)-15s %(message)s'
        datefmt: '%Y-%m-%d %H:%M:%S'
      custom:
          (): my.package.customFormatterFactory
          bar: baz
          spam: 99.9
          answer: 42

The above YAML snippet defines three formatters.  The first, with id
``brief``, is a standard :class:`logging.Formatter` instance with the
specified format string.  The second, with id ``default``, has a
longer format and also defines the time format explicitly, and will
result in a :class:`logging.Formatter` initialized with those two format
strings.  Shown in Python source form, the ``brief`` and ``default``
formatters have configuration sub-dictionaries::

    {
      'format' : '%(message)s'
    }

and::

    {
      'format' : '%(asctime)s %(levelname)-8s %(name)-15s %(message)s',
      'datefmt' : '%Y-%m-%d %H:%M:%S'
    }

respectively, and as these dictionaries do not contain the special key
``'()'``, the instantiation is inferred from the context: as a result,
standard :class:`logging.Formatter` instances are created.  The
configuration sub-dictionary for the third formatter, with id
``custom``, is::

  {
    '()' : 'my.package.customFormatterFactory',
    'bar' : 'baz',
    'spam' : 99.9,
    'answer' : 42
  }

and this contains the special key ``'()'``, which means that
user-defined instantiation is wanted.  In this case, the specified
factory callable will be used. If it is an actual callable it will be
used directly - otherwise, if you specify a string (as in the example)
the actual callable will be located using normal import mechanisms.
The callable will be called with the **remaining** items in the
configuration sub-dictionary as keyword arguments.  In the above
example, the formatter with id ``custom`` will be assumed to be
returned by the call::

    my.package.customFormatterFactory(bar='baz', spam=99.9, answer=42)

The key ``'()'`` has been used as the special key because it is not a
valid keyword parameter name, and so will not clash with the names of
the keyword arguments used in the call.  The ``'()'`` also serves as a
mnemonic that the corresponding value is a callable.


.. _logging-config-dict-externalobj:

Access to external objects
""""""""""""""""""""""""""

There are times where a configuration needs to refer to objects
external to the configuration, for example ``sys.stderr``.  If the
configuration dict is constructed using Python code, this is
straightforward, but a problem arises when the configuration is
provided via a text file (e.g. JSON, YAML).  In a text file, there is
no standard way to distinguish ``sys.stderr`` from the literal string
``'sys.stderr'``.  To facilitate this distinction, the configuration
system looks for certain special prefixes in string values and
treat them specially.  For example, if the literal string
``'ext://sys.stderr'`` is provided as a value in the configuration,
then the ``ext://`` will be stripped off and the remainder of the
value processed using normal import mechanisms.

The handling of such prefixes is done in a way analogous to protocol
handling: there is a generic mechanism to look for prefixes which
match the regular expression ``^(?P<prefix>[a-z]+)://(?P<suffix>.*)$``
whereby, if the ``prefix`` is recognised, the ``suffix`` is processed
in a prefix-dependent manner and the result of the processing replaces
the string value.  If the prefix is not recognised, then the string
value will be left as-is.


.. _logging-config-dict-internalobj:

Access to internal objects
""""""""""""""""""""""""""

As well as external objects, there is sometimes also a need to refer
to objects in the configuration.  This will be done implicitly by the
configuration system for things that it knows about.  For example, the
string value ``'DEBUG'`` for a ``level`` in a logger or handler will
automatically be converted to the value ``logging.DEBUG``, and the
``handlers``, ``filters`` and ``formatter`` entries will take an
object id and resolve to the appropriate destination object.

However, a more generic mechanism is needed for user-defined
objects which are not known to the :mod:`logging` module.  For
example, consider :class:`logging.handlers.MemoryHandler`, which takes
a ``target`` argument which is another handler to delegate to. Since
the system already knows about this class, then in the configuration,
the given ``target`` just needs to be the object id of the relevant
target handler, and the system will resolve to the handler from the
id.  If, however, a user defines a ``my.package.MyHandler`` which has
an ``alternate`` handler, the configuration system would not know that
the ``alternate`` referred to a handler.  To cater for this, a generic
resolution system allows the user to specify::

    handlers:
      file:
        # configuration of file handler goes here

      custom:
        (): my.package.MyHandler
        alternate: cfg://handlers.file

The literal string ``'cfg://handlers.file'`` will be resolved in an
analogous way to strings with the ``ext://`` prefix, but looking
in the configuration itself rather than the import namespace.  The
mechanism allows access by dot or by index, in a similar way to
that provided by ``str.format``.  Thus, given the following snippet::

    handlers:
      email:
        class: logging.handlers.SMTPHandler
        mailhost: localhost
        fromaddr: my_app@domain.tld
        toaddrs:
          - support_team@domain.tld
          - dev_team@domain.tld
        subject: Houston, we have a problem.

in the configuration, the string ``'cfg://handlers'`` would resolve to
the dict with key ``handlers``, the string ``'cfg://handlers.email``
would resolve to the dict with key ``email`` in the ``handlers`` dict,
and so on.  The string ``'cfg://handlers.email.toaddrs[1]`` would
resolve to ``'dev_team.domain.tld'`` and the string
``'cfg://handlers.email.toaddrs[0]'`` would resolve to the value
``'support_team@domain.tld'``. The ``subject`` value could be accessed
using either ``'cfg://handlers.email.subject'`` or, equivalently,
``'cfg://handlers.email[subject]'``.  The latter form only needs to be
used if the key contains spaces or non-alphanumeric characters.  If an
index value consists only of decimal digits, access will be attempted
using the corresponding integer value, falling back to the string
value if needed.

Given a string ``cfg://handlers.myhandler.mykey.123``, this will
resolve to ``config_dict['handlers']['myhandler']['mykey']['123']``.
If the string is specified as ``cfg://handlers.myhandler.mykey[123]``,
the system will attempt to retrieve the value from
``config_dict['handlers']['myhandler']['mykey'][123]``, and fall back
to ``config_dict['handlers']['myhandler']['mykey']['123']`` if that
fails.

.. _logging-config-fileformat:

Configuration file format
^^^^^^^^^^^^^^^^^^^^^^^^^

The configuration file format understood by :func:`fileConfig` is based on
:mod:`configparser` functionality. The file must contain sections called
``[loggers]``, ``[handlers]`` and ``[formatters]`` which identify by name the
entities of each type which are defined in the file. For each such entity, there
is a separate section which identifies how that entity is configured.  Thus, for
a logger named ``log01`` in the ``[loggers]`` section, the relevant
configuration details are held in a section ``[logger_log01]``. Similarly, a
handler called ``hand01`` in the ``[handlers]`` section will have its
configuration held in a section called ``[handler_hand01]``, while a formatter
called ``form01`` in the ``[formatters]`` section will have its configuration
specified in a section called ``[formatter_form01]``. The root logger
configuration must be specified in a section called ``[logger_root]``.

Examples of these sections in the file are given below. ::

   [loggers]
   keys=root,log02,log03,log04,log05,log06,log07

   [handlers]
   keys=hand01,hand02,hand03,hand04,hand05,hand06,hand07,hand08,hand09

   [formatters]
   keys=form01,form02,form03,form04,form05,form06,form07,form08,form09

The root logger must specify a level and a list of handlers. An example of a
root logger section is given below. ::

   [logger_root]
   level=NOTSET
   handlers=hand01

The ``level`` entry can be one of ``DEBUG, INFO, WARNING, ERROR, CRITICAL`` or
``NOTSET``. For the root logger only, ``NOTSET`` means that all messages will be
logged. Level values are :func:`eval`\ uated in the context of the ``logging``
package's namespace.

The ``handlers`` entry is a comma-separated list of handler names, which must
appear in the ``[handlers]`` section. These names must appear in the
``[handlers]`` section and have corresponding sections in the configuration
file.

For loggers other than the root logger, some additional information is required.
This is illustrated by the following example. ::

   [logger_parser]
   level=DEBUG
   handlers=hand01
   propagate=1
   qualname=compiler.parser

The ``level`` and ``handlers`` entries are interpreted as for the root logger,
except that if a non-root logger's level is specified as ``NOTSET``, the system
consults loggers higher up the hierarchy to determine the effective level of the
logger. The ``propagate`` entry is set to 1 to indicate that messages must
propagate to handlers higher up the logger hierarchy from this logger, or 0 to
indicate that messages are **not** propagated to handlers up the hierarchy. The
``qualname`` entry is the hierarchical channel name of the logger, that is to
say the name used by the application to get the logger.

Sections which specify handler configuration are exemplified by the following.
::

   [handler_hand01]
   class=StreamHandler
   level=NOTSET
   formatter=form01
   args=(sys.stdout,)

The ``class`` entry indicates the handler's class (as determined by :func:`eval`
in the ``logging`` package's namespace). The ``level`` is interpreted as for
loggers, and ``NOTSET`` is taken to mean 'log everything'.

.. versionchanged:: 2.6
   Added support for resolving the handler’s class as a dotted module and
   class name.

The ``formatter`` entry indicates the key name of the formatter for this
handler. If blank, a default formatter (``logging._defaultFormatter``) is used.
If a name is specified, it must appear in the ``[formatters]`` section and have
a corresponding section in the configuration file.

The ``args`` entry, when :func:`eval`\ uated in the context of the ``logging``
package's namespace, is the list of arguments to the constructor for the handler
class. Refer to the constructors for the relevant handlers, or to the examples
below, to see how typical entries are constructed. ::

   [handler_hand02]
   class=FileHandler
   level=DEBUG
   formatter=form02
   args=('python.log', 'w')

   [handler_hand03]
   class=handlers.SocketHandler
   level=INFO
   formatter=form03
   args=('localhost', handlers.DEFAULT_TCP_LOGGING_PORT)

   [handler_hand04]
   class=handlers.DatagramHandler
   level=WARN
   formatter=form04
   args=('localhost', handlers.DEFAULT_UDP_LOGGING_PORT)

   [handler_hand05]
   class=handlers.SysLogHandler
   level=ERROR
   formatter=form05
   args=(('localhost', handlers.SYSLOG_UDP_PORT), handlers.SysLogHandler.LOG_USER)

   [handler_hand06]
   class=handlers.NTEventLogHandler
   level=CRITICAL
   formatter=form06
   args=('Python Application', '', 'Application')

   [handler_hand07]
   class=handlers.SMTPHandler
   level=WARN
   formatter=form07
   args=('localhost', 'from@abc', ['user1@abc', 'user2@xyz'], 'Logger Subject')

   [handler_hand08]
   class=handlers.MemoryHandler
   level=NOTSET
   formatter=form08
   target=
   args=(10, ERROR)

   [handler_hand09]
   class=handlers.HTTPHandler
   level=NOTSET
   formatter=form09
   args=('localhost:9022', '/log', 'GET')

Sections which specify formatter configuration are typified by the following. ::

   [formatter_form01]
   format=F1 %(asctime)s %(levelname)s %(message)s
   datefmt=
   class=logging.Formatter

The ``format`` entry is the overall format string, and the ``datefmt`` entry is
the :func:`strftime`\ -compatible date/time format string.  If empty, the
package substitutes ISO8601 format date/times, which is almost equivalent to
specifying the date format string ``'%Y-%m-%d %H:%M:%S'``.  The ISO8601 format
also specifies milliseconds, which are appended to the result of using the above
format string, with a comma separator.  An example time in ISO8601 format is
``2003-01-23 00:29:50,411``.

The ``class`` entry is optional.  It indicates the name of the formatter's class
(as a dotted module and class name.)  This option is useful for instantiating a
:class:`Formatter` subclass.  Subclasses of :class:`Formatter` can present
exception tracebacks in an expanded or condensed format.

.. seealso::

   Module :mod:`logging`
      API reference for the logging module.

   Module :mod:`logging.handlers`
      Useful handlers included with the logging module.


