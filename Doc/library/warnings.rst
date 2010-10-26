:mod:`warnings` --- Warning control
===================================

.. index:: single: warnings

.. module:: warnings
   :synopsis: Issue warning messages and control their disposition.


Warning messages are typically issued in situations where it is useful to alert
the user of some condition in a program, where that condition (normally) doesn't
warrant raising an exception and terminating the program.  For example, one
might want to issue a warning when a program uses an obsolete module.

Python programmers issue warnings by calling the :func:`warn` function defined
in this module.  (C programmers use :c:func:`PyErr_WarnEx`; see
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
|                                  | features (ignored by default).                |
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
| :exc:`BytesWarning`              | Base category for warnings related to         |
|                                  | :class:`bytes` and :class:`buffer`.           |
+----------------------------------+-----------------------------------------------+
| :exc:`ResourceWarning`           | Base category for warnings related to         |
|                                  | resource usage.                               |
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
  must match (the match is compiled to always be case-insensitive).

* *category* is a class (a subclass of :exc:`Warning`) of which the warning
  category must be a subclass in order to match.

* *module* is a string containing a regular expression that the module name must
  match (the match is compiled to be case-sensitive).

* *lineno* is an integer that the line number where the warning occurred must
  match, or ``0`` to match all line numbers.

Since the :exc:`Warning` class is derived from the built-in :exc:`Exception`
class, to turn a warning into an error we simply raise ``category(message)``.

The warnings filter is initialized by :option:`-W` options passed to the Python
interpreter command line.  The interpreter saves the arguments for all
:option:`-W` options without interpretation in ``sys.warnoptions``; the
:mod:`warnings` module parses these when it is first imported (invalid options
are ignored, after printing a message to ``sys.stderr``).


Default Warning Filters
~~~~~~~~~~~~~~~~~~~~~~~

By default, Python installs several warning filters, which can be overridden by
the command-line options passed to :option:`-W` and calls to
:func:`filterwarnings`.

* :exc:`DeprecationWarning` and :exc:`PendingDeprecationWarning`, and
  :exc:`ImportWarning` are ignored.

* :exc:`BytesWarning` is ignored unless the :option:`-b` option is given once or
  twice; in this case this warning is either printed (``-b``) or turned into an
  exception (``-bb``).

* :exc:`ResourceWarning` is ignored unless Python was built in debug mode.

.. versionchanged:: 3.2
   :exc:`DeprecationWarning` is now ignored by default in addition to
   :exc:`PendingDeprecationWarning`.


.. _warning-suppress:

Temporarily Suppressing Warnings
--------------------------------

If you are using code that you know will raise a warning, such as a deprecated
function, but do not want to see the warning, then it is possible to suppress
the warning using the :class:`catch_warnings` context manager::

    import warnings

    def fxn():
        warnings.warn("deprecated", DeprecationWarning)

    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        fxn()

While within the context manager all warnings will simply be ignored. This
allows you to use known-deprecated code without having to see the warning while
not suppressing the warning for other code that might not be aware of its use
of deprecated code.  Note: this can only be guaranteed in a single-threaded
application. If two or more threads use the :class:`catch_warnings` context
manager at the same time, the behavior is undefined.



.. _warning-testing:

Testing Warnings
----------------

To test warnings raised by code, use the :class:`catch_warnings` context
manager. With it you can temporarily mutate the warnings filter to facilitate
your testing. For instance, do the following to capture all raised warnings to
check::

    import warnings

    def fxn():
        warnings.warn("deprecated", DeprecationWarning)

    with warnings.catch_warnings(record=True) as w:
        # Cause all warnings to always be triggered.
        warnings.simplefilter("always")
        # Trigger a warning.
        fxn()
        # Verify some things
        assert len(w) == 1
        assert issubclass(w[-1].category, DeprecationWarning)
        assert "deprecated" in str(w[-1].message)

One can also cause all warnings to be exceptions by using ``error`` instead of
``always``. One thing to be aware of is that if a warning has already been
raised because of a ``once``/``default`` rule, then no matter what filters are
set the warning will not be seen again unless the warnings registry related to
the warning has been cleared.

Once the context manager exits, the warnings filter is restored to its state
when the context was entered. This prevents tests from changing the warnings
filter in unexpected ways between tests and leading to indeterminate test
results. The :func:`showwarning` function in the module is also restored to
its original value.  Note: this can only be guaranteed in a single-threaded
application. If two or more threads use the :class:`catch_warnings` context
manager at the same time, the behavior is undefined.

When testing multiple operations that raise the same kind of warning, it
is important to test them in a manner that confirms each operation is raising
a new warning (e.g. set warnings to be raised as exceptions and check the
operations raise exceptions, check that the length of the warning list
continues to increase after each operation, or else delete the previous
entries from the warnings list before each new operation).


Updating Code For New Versions of Python
----------------------------------------

Warnings that are only of interest to the developer are ignored by default. As
such you should make sure to test your code with typically ignored warnings
made visible. You can do this from the command-line by passing :option:`-Wd`
to the interpreter (this is shorthand for :option:`-W default`).  This enables
default handling for all warnings, including those that are ignored by default.
To change what action is taken for encountered warnings you simply change what
argument is passed to :option:`-W`, e.g. :option:`-W error`. See the
:option:`-W` flag for more details on what is possible.

To programmatically do the same as :option:`-Wd`, use::

  warnings.simplefilter('default')

Make sure to execute this code as soon as possible. This prevents the
registering of what warnings have been raised from unexpectedly influencing how
future warnings are treated.

Having certain warnings ignored by default is done to prevent a user from
seeing warnings that are only of interest to the developer. As you do not
necessarily have control over what interpreter a user uses to run their code,
it is possible that a new version of Python will be released between your
release cycles.  The new interpreter release could trigger new warnings in your
code that were not there in an older interpreter, e.g.
:exc:`DeprecationWarning` for a module that you are using. While you as a
developer want to be notified that your code is using a deprecated module, to a
user this information is essentially noise and provides no benefit to them.


.. _warning-functions:

Available Functions
-------------------


.. function:: warn(message, category=None, stacklevel=1)

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


.. function:: warn_explicit(message, category, filename, lineno, module=None, registry=None, module_globals=None)

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
   source for modules found in zipfiles or other non-filesystem import
   sources).


.. function:: showwarning(message, category, filename, lineno, file=None, line=None)

   Write a warning to a file.  The default implementation calls
   ``formatwarning(message, category, filename, lineno, line)`` and writes the
   resulting string to *file*, which defaults to ``sys.stderr``.  You may replace
   this function with an alternative implementation by assigning to
   ``warnings.showwarning``.
   *line* is a line of source code to be included in the warning
   message; if *line* is not supplied, :func:`showwarning` will
   try to read the line specified by *filename* and *lineno*.


.. function:: formatwarning(message, category, filename, lineno, line=None)

   Format a warning the standard way.  This returns a string which may contain
   embedded newlines and ends in a newline.  *line* is a line of source code to
   be included in the warning message; if *line* is not supplied,
   :func:`formatwarning` will try to read the line specified by *filename* and
   *lineno*.


.. function:: filterwarnings(action, message='', category=Warning, module='', lineno=0, append=False)

   Insert an entry into the list of :ref:`warnings filter specifications
   <warning-filter>`.  The entry is inserted at the front by default; if
   *append* is true, it is inserted at the end.  This checks the types of the
   arguments, compiles the *message* and *module* regular expressions, and
   inserts them as a tuple in the list of warnings filters.  Entries closer to
   the front of the list override entries later in the list, if both match a
   particular warning.  Omitted arguments default to a value that matches
   everything.


.. function:: simplefilter(action, category=Warning, lineno=0, append=False)

   Insert a simple entry into the list of :ref:`warnings filter specifications
   <warning-filter>`.  The meaning of the function parameters is as for
   :func:`filterwarnings`, but regular expressions are not needed as the filter
   inserted always matches any message in any module as long as the category and
   line number match.


.. function:: resetwarnings()

   Reset the warnings filter.  This discards the effect of all previous calls to
   :func:`filterwarnings`, including that of the :option:`-W` command line options
   and calls to :func:`simplefilter`.


Available Context Managers
--------------------------

.. class:: catch_warnings(\*, record=False, module=None)

    A context manager that copies and, upon exit, restores the warnings filter
    and the :func:`showwarning` function.
    If the *record* argument is :const:`False` (the default) the context manager
    returns :class:`None` on entry. If *record* is :const:`True`, a list is
    returned that is progressively populated with objects as seen by a custom
    :func:`showwarning` function (which also suppresses output to ``sys.stdout``).
    Each object in the list has attributes with the same names as the arguments to
    :func:`showwarning`.

    The *module* argument takes a module that will be used instead of the
    module returned when you import :mod:`warnings` whose filter will be
    protected. This argument exists primarily for testing the :mod:`warnings`
    module itself.

    .. note::

        The :class:`catch_warnings` manager works by replacing and
        then later restoring the module's
        :func:`showwarning` function and internal list of filter
        specifications.  This means the context manager is modifying
        global state and therefore is not thread-safe.
