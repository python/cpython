:mod:`packaging.fancy_getopt` --- Wrapper around the getopt module
==================================================================

.. module:: packaging.fancy_getopt
   :synopsis: Additional getopt functionality.


.. warning::
   This module is deprecated and will be replaced with :mod:`optparse`.

This module provides a wrapper around the standard :mod:`getopt` module that
provides the following additional features:

* short and long options are tied together

* options have help strings, so :func:`fancy_getopt` could potentially create a
  complete usage summary

* options set attributes of a passed-in object

* boolean options can have "negative aliases" --- e.g. if :option:`--quiet` is
  the "negative alias" of :option:`--verbose`, then :option:`--quiet` on the
  command line sets *verbose* to false.

.. function:: fancy_getopt(options, negative_opt, object, args)

   Wrapper function. *options* is a list of ``(long_option, short_option,
   help_string)`` 3-tuples as described in the constructor for
   :class:`FancyGetopt`. *negative_opt* should be a dictionary mapping option names
   to option names, both the key and value should be in the *options* list.
   *object* is an object which will be used to store values (see the :meth:`getopt`
   method of the :class:`FancyGetopt` class). *args* is the argument list. Will use
   ``sys.argv[1:]`` if you pass ``None`` as *args*.


.. class:: FancyGetopt(option_table=None)

   The option_table is a list of 3-tuples: ``(long_option, short_option,
   help_string)``

   If an option takes an argument, its *long_option* should have ``'='`` appended;
   *short_option* should just be a single character, no ``':'`` in any case.
   *short_option* should be ``None`` if a *long_option* doesn't have a
   corresponding *short_option*. All option tuples must have long options.

The :class:`FancyGetopt` class provides the following methods:


.. method:: FancyGetopt.getopt(args=None, object=None)

   Parse command-line options in args. Store as attributes on *object*.

   If *args* is ``None`` or not supplied, uses ``sys.argv[1:]``.  If *object* is
   ``None`` or not supplied, creates a new :class:`OptionDummy` instance, stores
   option values there, and returns a tuple ``(args, object)``.  If *object* is
   supplied, it is modified in place and :func:`getopt` just returns *args*; in
   both cases, the returned *args* is a modified copy of the passed-in *args* list,
   which is left untouched.

   .. TODO and args returned are?


.. method:: FancyGetopt.get_option_order()

   Returns the list of ``(option, value)`` tuples processed by the previous run of
   :meth:`getopt`  Raises :exc:`RuntimeError` if :meth:`getopt` hasn't been called
   yet.


.. method:: FancyGetopt.generate_help(header=None)

   Generate help text (a list of strings, one per suggested line of output) from
   the option table for this :class:`FancyGetopt` object.

   If supplied, prints the supplied *header* at the top of the help.
