
:mod:`warnings` --- Warning control
===================================

.. index:: single: warnings

.. module:: warnings
   :synopsis: Issue warning messages and control their disposition.


.. versionadded:: 2.1

Warning messages are typically issued in situations where it is useful to alert
the user of some condition in a program, where that condition (normally) doesn't
warrant raising an exception and terminating the program.  For example, one
might want to issue a warning when a program uses an obsolete module.

Python programmers issue warnings by calling the :func:`warn` function defined
in this module.  (C programmers use :cfunc:`PyErr_WarnEx`; see
:ref:`exceptionhandling` for details).

Warning messages are normally written to ``sys.stderr``, but their disposition
can be changed flexibly, from ignoring all warnings to turning them into
exceptions.  The disposition of warnings can vary based on the warning category
(see below), the text of the warning message, and the source location where it
is issued.  Repetitions of a particular warning for the same source location are
typically suppressed.

There are two stages in warning control: first, each time a warning is issued, a
determination is made whether a message should be issued or not; next, if a
message is to be issued, it is formatted and printed using a user-settable hook.

The determination whether to issue a warning message is controlled by the
warning filter, which is a sequence of matching rules and actions. Rules can be
added to the filter by calling :func:`filterwarnings` and reset to its default
state by calling :func:`resetwarnings`.

The printing of warning messages is done by calling :func:`showwarning`, which
may be overridden; the default implementation of this function formats the
message by calling :func:`formatwarning`, which is also available for use by
custom implementations.


.. _warning-categories:

Warning Categories
------------------

There are a number of built-in exceptions that represent warning categories.
This categorization is useful to be able to filter out groups of warnings.  The
following warnings category classes are currently defined:

+----------------------------------+-----------------------------------------------+
| Class                            | Description                                   |
+==================================+===============================================+
| :exc:`Warning`                   | This is the base class of all warning         |
|                                  | category classes.  It is a subclass of        |
|                                  | :exc:`Exception`.                             |
+----------------------------------+-----------------------------------------------+
| :exc:`UserWarning`               | The default category for :func:`warn`.        |
+----------------------------------+-----------------------------------------------+
| :exc:`DeprecationWarning`        | Base category for warnings about deprecated   |
|                                  | features.                                     |
+----------------------------------+-----------------------------------------------+
| :exc:`SyntaxWarning`             | Base category for warnings about dubious      |
|                                  | syntactic features.                           |
+----------------------------------+-----------------------------------------------+
| :exc:`RuntimeWarning`            | Base category for warnings about dubious      |
|                                  | runtime features.                             |
+----------------------------------+-----------------------------------------------+
| :exc:`FutureWarning`             | Base category for warnings about constructs   |
|                                  | that will change semantically in the future.  |
+----------------------------------+-----------------------------------------------+
| :exc:`PendingDeprecationWarning` | Base category for warnings about features     |
|                                  | that will be deprecated in the future         |
|                                  | (ignored by default).                         |
+----------------------------------+-----------------------------------------------+
| :exc:`ImportWarning`             | Base category for warnings triggered during   |
|                                  | the process of importing a module (ignored by |
|                                  | default).                                     |
+----------------------------------+-----------------------------------------------+
| :exc:`UnicodeWarning`            | Base category for warnings related to         |
|                                  | Unicode.                                      |
+----------------------------------+-----------------------------------------------+

While these are technically built-in exceptions, they are documented here,
because conceptually they belong to the warnings mechanism.

User code can define additional warning categories by subclassing one of the
standard warning categories.  A warning category must always be a subclass of
the :exc:`Warning` class.


.. _warning-filter:

The Warnings Filter
-------------------

The warnings filter controls whether warnings are ignored, displayed, or turned
into errors (raising an exception).

Conceptually, the warnings filter maintains an ordered list of filter
specifications; any specific warning is matched against each filter
specification in the list in turn until a match is found; the match determines
the disposition of the match.  Each entry is a tuple of the form (*action*,
*message*, *category*, *module*, *lineno*), where:

* *action* is one of the following strings:

  +---------------+----------------------------------------------+
  | Value         | Disposition                                  |
  +===============+==============================================+
  | ``"error"``   | turn matching warnings into exceptions       |
  +---------------+----------------------------------------------+
  | ``"ignore"``  | never print matching warnings                |
  +---------------+----------------------------------------------+
  | ``"always"``  | always print matching warnings               |
  +---------------+----------------------------------------------+
  | ``"default"`` | print the first occurrence of matching       |
  |               | warnings for each location where the warning |
  |               | is issued                                    |
  +---------------+----------------------------------------------+
  | ``"module"``  | print the first occurrence of matching       |
  |               | warnings for each module where the warning   |
  |               | is issued                                    |
  +---------------+----------------------------------------------+
  | ``"once"``    | print only the first occurrence of matching  |
  |               | warnings, regardless of location             |
  +---------------+----------------------------------------------+

* *message* is a string containing a regular expression that the warning message
  must match (the match is compiled to always be  case-insensitive)

* *category* is a class (a subclass of :exc:`Warning`) of which the warning
  category must be a subclass in order to match

* *module* is a string containing a regular expression that the module name must
  match (the match is compiled to be case-sensitive)

* *lineno* is an integer that the line number where the warning occurred must
  match, or ``0`` to match all line numbers

Since the :exc:`Warning` class is derived from the built-in :exc:`Exception`
class, to turn a warning into an error we simply raise ``category(message)``.

The warnings filter is initialized by :option:`-W` options passed to the Python
interpreter command line.  The interpreter saves the arguments for all
:option:`-W` options without interpretation in ``sys.warnoptions``; the
:mod:`warnings` module parses these when it is first imported (invalid options
are ignored, after printing a message to ``sys.stderr``).

The warnings that are ignored by default may be enabled by passing :option:`-Wd`
to the interpreter. This enables default handling for all warnings, including
those that are normally ignored by default. This is particular useful for
enabling ImportWarning when debugging problems importing a developed package.
ImportWarning can also be enabled explicitly in Python code using::

   warnings.simplefilter('default', ImportWarning)


.. _warning-functions:

Available Functions
-------------------


.. function:: warn(message[, category[, stacklevel]])

   Issue a warning, or maybe ignore it or raise an exception.  The *category*
   argument, if given, must be a warning category class (see above); it defaults to
   :exc:`UserWarning`.  Alternatively *message* can be a :exc:`Warning` instance,
   in which case *category* will be ignored and ``message.__class__`` will be used.
   In this case the message text will be ``str(message)``. This function raises an
   exception if the particular warning issued is changed into an error by the
   warnings filter see above.  The *stacklevel* argument can be used by wrapper
   functions written in Python, like this::

      def deprecation(message):
          warnings.warn(message, DeprecationWarning, stacklevel=2)

   This makes the warning refer to :func:`deprecation`'s caller, rather than to the
   source of :func:`deprecation` itself (since the latter would defeat the purpose
   of the warning message).


.. function:: warn_explicit(message, category, filename, lineno[, module[, registry[, module_globals]]])

   This is a low-level interface to the functionality of :func:`warn`, passing in
   explicitly the message, category, filename and line number, and optionally the
   module name and the registry (which should be the ``__warningregistry__``
   dictionary of the module).  The module name defaults to the filename with
   ``.py`` stripped; if no registry is passed, the warning is never suppressed.
   *message* must be a string and *category* a subclass of :exc:`Warning` or
   *message* may be a :exc:`Warning` instance, in which case *category* will be
   ignored.

   *module_globals*, if supplied, should be the global namespace in use by the code
   for which the warning is issued.  (This argument is used to support displaying
   source for modules found in zipfiles or other non-filesystem import sources, and
   was added in Python 2.5.)


.. function:: showwarning(message, category, filename, lineno[, file])

   Write a warning to a file.  The default implementation calls
   ``formatwarning(message, category, filename, lineno)`` and writes the resulting
   string to *file*, which defaults to ``sys.stderr``.  You may replace this
   function with an alternative implementation by assigning to
   ``warnings.showwarning``.


.. function:: formatwarning(message, category, filename, lineno)

   Format a warning the standard way.  This returns a string  which may contain
   embedded newlines and ends in a newline.


.. function:: filterwarnings(action[, message[, category[, module[, lineno[, append]]]]])

   Insert an entry into the list of warnings filters.  The entry is inserted at the
   front by default; if *append* is true, it is inserted at the end. This checks
   the types of the arguments, compiles the message and module regular expressions,
   and inserts them as a tuple in the  list of warnings filters.  Entries closer to
   the front of the list override entries later in the list, if both match a
   particular warning.  Omitted arguments default to a value that matches
   everything.


.. function:: simplefilter(action[, category[, lineno[, append]]])

   Insert a simple entry into the list of warnings filters. The meaning of the
   function parameters is as for :func:`filterwarnings`, but regular expressions
   are not needed as the filter inserted always matches any message in any module
   as long as the category and line number match.


.. function:: resetwarnings()

   Reset the warnings filter.  This discards the effect of all previous calls to
   :func:`filterwarnings`, including that of the :option:`-W` command line options
   and calls to :func:`simplefilter`.

