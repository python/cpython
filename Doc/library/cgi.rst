:mod:`!cgi` --- Common Gateway Interface support
================================================

.. module:: cgi
   :synopsis: Removed in 3.13.
   :deprecated:

This module is no longer part of the Python standard library.
It was :ref:`removed in Python 3.13 <whatsnew313-pep594>` after
being deprecated in Python 3.11.  The removal was decided in :pep:`594`.

A fork of the module on PyPI can be used instead: :pypi:`legacy-cgi`.

If you want to update your code using supported standard library modules:

* ``cgi.FieldStorage`` can typically be replaced with
  :func:`urllib.parse.parse_qsl` for ``GET`` and ``HEAD`` requests, and the
  :mod:`email.message` module or :pypi:`multipart` PyPI project for ``POST``
  and ``PUT``.

* ``cgi.parse()`` can be replaced by calling :func:`urllib.parse.parse_qs`
  directly on the desired query string, except for ``multipart/form-data``
  input, which can be handled as described for ``cgi.parse_multipart()``.

* ``cgi.parse_multipart()`` can be replaced with the functionality in the
  :mod:`email` package (e.g. :class:`email.message.EmailMessage` and
  :class:`email.message.Message`) which implements the same MIME RFCs, or
  with the :pypi:`multipart` PyPI project.

* ``cgi.parse_header()`` can be replaced with the functionality in the
  :mod:`email` package, which implements the same MIME RFCs. For example,
  with :class:`email.message.EmailMessage`::

      from email.message import EmailMessage
      msg = EmailMessage()
      msg['content-type'] = 'application/json; charset="utf8"'
      main, params = msg.get_content_type(), msg['content-type'].params
