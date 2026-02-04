Pending removal in future versions
----------------------------------

The following APIs will be removed in the future,
although there is currently no date scheduled for their removal.

* :mod:`argparse`:

  * Nesting argument groups and nesting mutually exclusive
    groups are deprecated.
  * Passing the undocumented keyword argument *prefix_chars* to
    :meth:`~argparse.ArgumentParser.add_argument_group` is now
    deprecated.
  * The :class:`argparse.FileType` type converter is deprecated.

* :mod:`builtins`:

  * Generators: ``throw(type, exc, tb)`` and ``athrow(type, exc, tb)``
    signature is deprecated: use ``throw(exc)`` and ``athrow(exc)`` instead,
    the single argument signature.
  * Currently Python accepts numeric literals immediately followed by keywords,
    for example ``0in x``, ``1or x``, ``0if 1else 2``.  It allows confusing and
    ambiguous expressions like ``[0x1for x in y]`` (which can be interpreted as
    ``[0x1 for x in y]`` or ``[0x1f or x in y]``).  A syntax warning is raised
    if the numeric literal is immediately followed by one of keywords
    :keyword:`and`, :keyword:`else`, :keyword:`for`, :keyword:`if`,
    :keyword:`in`, :keyword:`is` and :keyword:`or`.  In a future release it
    will be changed to a syntax error. (:gh:`87999`)
  * Support for ``__index__()`` and ``__int__()`` method returning non-int type:
    these methods will be required to return an instance of a strict subclass of
    :class:`int`.
  * Support for ``__float__()`` method returning a strict subclass of
    :class:`float`: these methods will be required to return an instance of
    :class:`float`.
  * Support for ``__complex__()`` method returning a strict subclass of
    :class:`complex`: these methods will be required to return an instance of
    :class:`complex`.
  * Delegation of ``int()`` to ``__trunc__()`` method.
  * Passing a complex number as the *real* or *imag* argument in the
    :func:`complex` constructor is now deprecated; it should only be passed
    as a single positional argument.
    (Contributed by Serhiy Storchaka in :gh:`109218`.)

* :mod:`calendar`: ``calendar.January`` and ``calendar.February`` constants are
  deprecated and replaced by :data:`calendar.JANUARY` and
  :data:`calendar.FEBRUARY`.
  (Contributed by Prince Roshan in :gh:`103636`.)

* :mod:`codecs`: use :func:`open` instead of :func:`codecs.open`. (:gh:`133038`)

* :attr:`codeobject.co_lnotab`: use the :meth:`codeobject.co_lines` method
  instead.

* :mod:`datetime`:

  * :meth:`~datetime.datetime.utcnow`:
    use ``datetime.datetime.now(tz=datetime.UTC)``.
  * :meth:`~datetime.datetime.utcfromtimestamp`:
    use ``datetime.datetime.fromtimestamp(timestamp, tz=datetime.UTC)``.

* :mod:`gettext`: Plural value must be an integer.

* :mod:`importlib`:

  * :func:`~importlib.util.cache_from_source` *debug_override* parameter is
    deprecated: use the *optimization* parameter instead.

* :mod:`importlib.metadata`:

  * ``EntryPoints`` tuple interface.
  * Implicit ``None`` on return values.

* :mod:`logging`: the ``warn()`` method has been deprecated
  since Python 3.3, use :meth:`~logging.warning` instead.

* :mod:`mailbox`: Use of StringIO input and text mode is deprecated, use
  BytesIO and binary mode instead.

* :mod:`os`: Calling :func:`os.register_at_fork` in a multi-threaded process.

* :class:`!pydoc.ErrorDuringImport`: A tuple value for *exc_info* parameter is
  deprecated, use an exception instance.

* :mod:`re`: More strict rules are now applied for numerical group references
  and group names in regular expressions.  Only sequence of ASCII digits is now
  accepted as a numerical reference.  The group name in bytes patterns and
  replacement strings can now only contain ASCII letters and digits and
  underscore.
  (Contributed by Serhiy Storchaka in :gh:`91760`.)

* :mod:`shutil`: :func:`~shutil.rmtree`'s *onerror* parameter is deprecated in
  Python 3.12; use the *onexc* parameter instead.

* :mod:`ssl` options and protocols:

  * :class:`ssl.SSLContext` without protocol argument is deprecated.
  * :class:`ssl.SSLContext`: :meth:`~ssl.SSLContext.set_npn_protocols` and
    :meth:`!selected_npn_protocol` are deprecated: use ALPN
    instead.
  * ``ssl.OP_NO_SSL*`` options
  * ``ssl.OP_NO_TLS*`` options
  * ``ssl.PROTOCOL_SSLv3``
  * ``ssl.PROTOCOL_TLS``
  * ``ssl.PROTOCOL_TLSv1``
  * ``ssl.PROTOCOL_TLSv1_1``
  * ``ssl.PROTOCOL_TLSv1_2``
  * ``ssl.TLSVersion.SSLv3``
  * ``ssl.TLSVersion.TLSv1``
  * ``ssl.TLSVersion.TLSv1_1``

* :mod:`threading` methods:

  * :meth:`!threading.Condition.notifyAll`: use :meth:`~threading.Condition.notify_all`.
  * :meth:`!threading.Event.isSet`: use :meth:`~threading.Event.is_set`.
  * :meth:`!threading.Thread.isDaemon`, :meth:`threading.Thread.setDaemon`:
    use :attr:`threading.Thread.daemon` attribute.
  * :meth:`!threading.Thread.getName`, :meth:`threading.Thread.setName`:
    use :attr:`threading.Thread.name` attribute.
  * :meth:`!threading.currentThread`: use :meth:`threading.current_thread`.
  * :meth:`!threading.activeCount`: use :meth:`threading.active_count`.

* :class:`typing.Text` (:gh:`92332`).

* The internal class ``typing._UnionGenericAlias`` is no longer used to implement
  :class:`typing.Union`. To preserve compatibility with users using this private
  class, a compatibility shim will be provided until at least Python 3.17. (Contributed by
  Jelle Zijlstra in :gh:`105499`.)

* :class:`unittest.IsolatedAsyncioTestCase`: it is deprecated to return a value
  that is not ``None`` from a test case.

* :mod:`urllib.parse` deprecated functions: :func:`~urllib.parse.urlparse` instead

  * ``splitattr()``
  * ``splithost()``
  * ``splitnport()``
  * ``splitpasswd()``
  * ``splitport()``
  * ``splitquery()``
  * ``splittag()``
  * ``splittype()``
  * ``splituser()``
  * ``splitvalue()``
  * ``to_bytes()``

* :mod:`wsgiref`: ``SimpleHandler.stdout.write()`` should not do partial
  writes.

* :mod:`xml.etree.ElementTree`: Testing the truth value of an
  :class:`~xml.etree.ElementTree.Element` is deprecated. In a future release it
  will always return ``True``. Prefer explicit ``len(elem)`` or
  ``elem is not None`` tests instead.

* :func:`sys._clear_type_cache` is deprecated:
  use :func:`sys._clear_internal_caches` instead.
