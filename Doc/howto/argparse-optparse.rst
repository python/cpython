.. currentmodule:: argparse

.. _upgrading-optparse-code:

==========================
Upgrading optparse code
==========================

Originally, the :mod:`argparse` module had attempted to maintain compatibility
with :mod:`optparse`.  However, :mod:`optparse` was difficult to extend
transparently, particularly with the changes required to support
``nargs=`` specifiers and better usage messages.  When most everything in
:mod:`optparse` had either been copy-pasted over or monkey-patched, it no
longer seemed practical to try to maintain the backwards compatibility.

The :mod:`argparse` module improves on the :mod:`optparse`
module in a number of ways including:

* Handling positional arguments.
* Supporting subcommands.
* Allowing alternative option prefixes like ``+`` and ``/``.
* Handling zero-or-more and one-or-more style arguments.
* Producing more informative usage messages.
* Providing a much simpler interface for custom ``type`` and ``action``.

A partial upgrade path from :mod:`optparse` to :mod:`argparse`:

* Replace all :meth:`optparse.OptionParser.add_option` calls with
  :meth:`ArgumentParser.add_argument` calls.

* Replace ``(options, args) = parser.parse_args()`` with ``args =
  parser.parse_args()`` and add additional :meth:`ArgumentParser.add_argument`
  calls for the positional arguments. Keep in mind that what was previously
  called ``options``, now in the :mod:`argparse` context is called ``args``.

* Replace :meth:`optparse.OptionParser.disable_interspersed_args`
  by using :meth:`~ArgumentParser.parse_intermixed_args` instead of
  :meth:`~ArgumentParser.parse_args`.

* Replace callback actions and the ``callback_*`` keyword arguments with
  ``type`` or ``action`` arguments.

* Replace string names for ``type`` keyword arguments with the corresponding
  type objects (e.g. int, float, complex, etc).

* Replace :class:`optparse.Values` with :class:`Namespace` and
  :exc:`optparse.OptionError` and :exc:`optparse.OptionValueError` with
  :exc:`ArgumentError`.

* Replace strings with implicit arguments such as ``%default`` or ``%prog`` with
  the standard Python syntax to use dictionaries to format strings, that is,
  ``%(default)s`` and ``%(prog)s``.

* Replace the OptionParser constructor ``version`` argument with a call to
  ``parser.add_argument('--version', action='version', version='<the version>')``.
