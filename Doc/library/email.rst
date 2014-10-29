:mod:`email` --- An email and MIME handling package
===================================================

.. module:: email
   :synopsis: Package supporting the parsing, manipulating, and generating email messages,
              including MIME documents.
.. moduleauthor:: Barry A. Warsaw <barry@python.org>
.. sectionauthor:: Barry A. Warsaw <barry@python.org>
.. Copyright (C) 2001-2007 Python Software Foundation


.. versionadded:: 2.2

The :mod:`email` package is a library for managing email messages, including
MIME and other :rfc:`2822`\ -based message documents.  It subsumes most of the
functionality in several older standard modules such as :mod:`rfc822`,
:mod:`mimetools`, :mod:`multifile`, and other non-standard packages such as
:mod:`mimecntl`.  It is specifically *not* designed to do any sending of email
messages to SMTP (:rfc:`2821`), NNTP, or other servers; those are functions of
modules such as :mod:`smtplib` and :mod:`nntplib`. The :mod:`email` package
attempts to be as RFC-compliant as possible, supporting in addition to
:rfc:`2822`, such MIME-related RFCs as :rfc:`2045`, :rfc:`2046`, :rfc:`2047`,
and :rfc:`2231`.

The primary distinguishing feature of the :mod:`email` package is that it splits
the parsing and generating of email messages from the internal *object model*
representation of email.  Applications using the :mod:`email` package deal
primarily with objects; you can add sub-objects to messages, remove sub-objects
from messages, completely re-arrange the contents, etc.  There is a separate
parser and a separate generator which handles the transformation from flat text
to the object model, and then back to flat text again.  There are also handy
subclasses for some common MIME object types, and a few miscellaneous utilities
that help with such common tasks as extracting and parsing message field values,
creating RFC-compliant dates, etc.

The following sections describe the functionality of the :mod:`email` package.
The ordering follows a progression that should be common in applications: an
email message is read as flat text from a file or other source, the text is
parsed to produce the object structure of the email message, this structure is
manipulated, and finally, the object tree is rendered back into flat text.

It is perfectly feasible to create the object structure out of whole cloth ---
i.e. completely from scratch.  From there, a similar progression can be taken as
above.

Also included are detailed specifications of all the classes and modules that
the :mod:`email` package provides, the exception classes you might encounter
while using the :mod:`email` package, some auxiliary utilities, and a few
examples.  For users of the older :mod:`mimelib` package, or previous versions
of the :mod:`email` package, a section on differences and porting is provided.

Contents of the :mod:`email` package documentation:

.. toctree::

   email.message.rst
   email.parser.rst
   email.generator.rst
   email.mime.rst
   email.header.rst
   email.charset.rst
   email.encoders.rst
   email.errors.rst
   email.util.rst
   email.iterators.rst
   email-examples.rst


.. seealso::

   Module :mod:`smtplib`
      SMTP protocol client

   Module :mod:`nntplib`
      NNTP protocol client


.. _email-pkg-history:

Package History
---------------

This table describes the release history of the email package, corresponding to
the version of Python that the package was released with.  For purposes of this
document, when you see a note about change or added versions, these refer to the
Python version the change was made in, *not* the email package version.  This
table also describes the Python compatibility of each version of the package.

+---------------+------------------------------+-----------------------+
| email version | distributed with             | compatible with       |
+===============+==============================+=======================+
| :const:`1.x`  | Python 2.2.0 to Python 2.2.1 | *no longer supported* |
+---------------+------------------------------+-----------------------+
| :const:`2.5`  | Python 2.2.2+ and Python 2.3 | Python 2.1 to 2.5     |
+---------------+------------------------------+-----------------------+
| :const:`3.0`  | Python 2.4                   | Python 2.3 to 2.5     |
+---------------+------------------------------+-----------------------+
| :const:`4.0`  | Python 2.5                   | Python 2.3 to 2.5     |
+---------------+------------------------------+-----------------------+

Here are the major differences between :mod:`email` version 4 and version 3:

* All modules have been renamed according to :pep:`8` standards.  For example,
  the version 3 module :mod:`email.Message` was renamed to :mod:`email.message` in
  version 4.

* A new subpackage :mod:`email.mime` was added and all the version 3
  :mod:`email.MIME\*` modules were renamed and situated into the :mod:`email.mime`
  subpackage.  For example, the version 3 module :mod:`email.MIMEText` was renamed
  to :mod:`email.mime.text`.

  *Note that the version 3 names will continue to work until Python 2.6*.

* The :mod:`email.mime.application` module was added, which contains the
  :class:`~email.mime.application.MIMEApplication` class.

* Methods that were deprecated in version 3 have been removed.  These include
  :meth:`Generator.__call__`, :meth:`Message.get_type`,
  :meth:`Message.get_main_type`, :meth:`Message.get_subtype`.

* Fixes have been added for :rfc:`2231` support which can change some of the
  return types for :func:`Message.get_param <email.message.Message.get_param>`
  and friends.  Under some
  circumstances, values which used to return a 3-tuple now return simple strings
  (specifically, if all extended parameter segments were unencoded, there is no
  language and charset designation expected, so the return type is now a simple
  string).  Also, %-decoding used to be done for both encoded and unencoded
  segments; this decoding is now done only for encoded segments.

Here are the major differences between :mod:`email` version 3 and version 2:

* The :class:`~email.parser.FeedParser` class was introduced, and the
  :class:`~email.parser.Parser` class was implemented in terms of the
  :class:`~email.parser.FeedParser`.  All parsing therefore is
  non-strict, and parsing will make a best effort never to raise an exception.
  Problems found while parsing messages are stored in the message's *defect*
  attribute.

* All aspects of the API which raised :exc:`DeprecationWarning`\ s in version 2
  have been removed.  These include the *_encoder* argument to the
  :class:`~email.mime.text.MIMEText` constructor, the
  :meth:`Message.add_payload` method, the :func:`Utils.dump_address_pair`
  function, and the functions :func:`Utils.decode` and :func:`Utils.encode`.

* New :exc:`DeprecationWarning`\ s have been added to:
  :meth:`Generator.__call__`, :meth:`Message.get_type`,
  :meth:`Message.get_main_type`, :meth:`Message.get_subtype`, and the *strict*
  argument to the :class:`~email.parser.Parser` class.  These are expected to
  be removed in future versions.

* Support for Pythons earlier than 2.3 has been removed.

Here are the differences between :mod:`email` version 2 and version 1:

* The :mod:`email.Header` and :mod:`email.Charset` modules have been added.

* The pickle format for :class:`~email.message.Message` instances has changed.
  Since this was never (and still isn't) formally defined, this isn't
  considered a backward incompatibility.  However if your application pickles
  and unpickles :class:`~email.message.Message` instances, be aware that in
  :mod:`email` version 2, :class:`~email.message.Message` instances now have
  private variables *_charset* and *_default_type*.

* Several methods in the :class:`~email.message.Message` class have been
  deprecated, or their signatures changed.  Also, many new methods have been
  added.  See the documentation for the :class:`~email.message.Message` class
  for details.  The changes should be completely backward compatible.

* The object structure has changed in the face of :mimetype:`message/rfc822`
  content types.  In :mod:`email` version 1, such a type would be represented
  by a scalar payload, i.e. the container message's
  :meth:`~email.message.Message.is_multipart` returned false,
  :meth:`~email.message.Message.get_payload` was not a list object, but a
  single :class:`~email.message.Message` instance.

  This structure was inconsistent with the rest of the package, so the object
  representation for :mimetype:`message/rfc822` content types was changed.  In
  :mod:`email` version 2, the container *does* return ``True`` from
  :meth:`~email.message.Message.is_multipart`, and
  :meth:`~email.message.Message.get_payload` returns a list containing a single
  :class:`~email.message.Message` item.

  Note that this is one place that backward compatibility could not be
  completely maintained.  However, if you're already testing the return type of
  :meth:`~email.message.Message.get_payload`, you should be fine.  You just need
  to make sure your code doesn't do a :meth:`~email.message.Message.set_payload`
  with a :class:`~email.message.Message` instance on a container with a content
  type of :mimetype:`message/rfc822`.

* The :class:`~email.parser.Parser` constructor's *strict* argument was added,
  and its :meth:`~email.parser.Parser.parse` and
  :meth:`~email.parser.Parser.parsestr` methods grew a *headersonly* argument.
  The *strict* flag was also added to functions :func:`email.message_from_file`
  and :func:`email.message_from_string`.

* :meth:`Generator.__call__` is deprecated; use :meth:`Generator.flatten
  <email.generator.Generator.flatten>` instead.  The
  :class:`~email.generator.Generator` class has also grown the
  :meth:`~email.generator.Generator.clone` method.

* The :class:`~email.generator.DecodedGenerator` class in the
  :mod:`email.generator` module was added.

* The intermediate base classes
  :class:`~email.mime.nonmultipart.MIMENonMultipart` and
  :class:`~email.mime.multipart.MIMEMultipart` have been added, and interposed
  in the class hierarchy for most of the other MIME-related derived classes.

* The *_encoder* argument to the :class:`~email.mime.text.MIMEText` constructor
  has been deprecated.  Encoding  now happens implicitly based on the
  *_charset* argument.

* The following functions in the :mod:`email.Utils` module have been deprecated:
  :func:`dump_address_pairs`, :func:`decode`, and :func:`encode`.  The following
  functions have been added to the module: :func:`make_msgid`,
  :func:`decode_rfc2231`, :func:`encode_rfc2231`, and :func:`decode_params`.

* The non-public function :func:`email.Iterators._structure` was added.


Differences from :mod:`mimelib`
-------------------------------

The :mod:`email` package was originally prototyped as a separate library called
`mimelib <http://mimelib.sourceforge.net/>`_. Changes have been made so that method names
are more consistent, and some methods or modules have either been added or
removed.  The semantics of some of the methods have also changed.  For the most
part, any functionality available in :mod:`mimelib` is still available in the
:mod:`email` package, albeit often in a different way.  Backward compatibility
between the :mod:`mimelib` package and the :mod:`email` package was not a
priority.

Here is a brief description of the differences between the :mod:`mimelib` and
the :mod:`email` packages, along with hints on how to port your applications.

Of course, the most visible difference between the two packages is that the
package name has been changed to :mod:`email`.  In addition, the top-level
package has the following differences:

* :func:`messageFromString` has been renamed to :func:`message_from_string`.

* :func:`messageFromFile` has been renamed to :func:`message_from_file`.

The :class:`~email.message.Message` class has the following differences:

* The method :meth:`asString` was renamed to
  :meth:`~email.message.Message.as_string`.

* The method :meth:`ismultipart` was renamed to
  :meth:`~email.message.Message.is_multipart`.

* The :meth:`~email.message.Message.get_payload` method has grown a *decode*
  optional argument.

* The method :meth:`getall` was renamed to
  :meth:`~email.message.Message.get_all`.

* The method :meth:`addheader` was renamed to
  :meth:`~email.message.Message.add_header`.

* The method :meth:`gettype` was renamed to :meth:`get_type`.

* The method :meth:`getmaintype` was renamed to :meth:`get_main_type`.

* The method :meth:`getsubtype` was renamed to :meth:`get_subtype`.

* The method :meth:`getparams` was renamed to
  :meth:`~email.message.Message.get_params`. Also, whereas :meth:`getparams`
  returned a list of strings, :meth:`~email.message.Message.get_params` returns
  a list of 2-tuples, effectively the key/value pairs of the parameters, split
  on the ``'='`` sign.

* The method :meth:`getparam` was renamed to
  :meth:`~email.message.Message.get_param`.

* The method :meth:`getcharsets` was renamed to
  :meth:`~email.message.Message.get_charsets`.

* The method :meth:`getfilename` was renamed to
  :meth:`~email.message.Message.get_filename`.

* The method :meth:`getboundary` was renamed to
  :meth:`~email.message.Message.get_boundary`.

* The method :meth:`setboundary` was renamed to
  :meth:`~email.message.Message.set_boundary`.

* The method :meth:`getdecodedpayload` was removed.  To get similar
  functionality, pass the value 1 to the *decode* flag of the
  :meth:`~email.message.Message.get_payload` method.

* The method :meth:`getpayloadastext` was removed.  Similar functionality is
  supported by the :class:`~email.generator.DecodedGenerator` class in the
  :mod:`email.generator` module.

* The method :meth:`getbodyastext` was removed.  You can get similar
  functionality by creating an iterator with
  :func:`~email.iterators.typed_subpart_iterator` in the :mod:`email.iterators`
  module.

The :class:`~email.parser.Parser` class has no differences in its public
interface. It does have some additional smarts to recognize
:mimetype:`message/delivery-status` type messages, which it represents as a
:class:`~email.message.Message` instance containing separate
:class:`~email.message.Message` subparts for each header block in the delivery
status notification [#]_.

The :class:`~email.generator.Generator` class has no differences in its public
interface.  There is a new class in the :mod:`email.generator` module though,
called :class:`~email.generator.DecodedGenerator` which provides most of the
functionality previously available in the :meth:`Message.getpayloadastext`
method.

The following modules and classes have been changed:

* The :class:`~email.mime.base.MIMEBase` class constructor arguments *_major*
  and *_minor* have changed to *_maintype* and *_subtype* respectively.

* The ``Image`` class/module has been renamed to ``MIMEImage``.  The *_minor*
  argument has been renamed to *_subtype*.

* The ``Text`` class/module has been renamed to ``MIMEText``.  The *_minor*
  argument has been renamed to *_subtype*.

* The ``MessageRFC822`` class/module has been renamed to ``MIMEMessage``.  Note
  that an earlier version of :mod:`mimelib` called this class/module ``RFC822``,
  but that clashed with the Python standard library module :mod:`rfc822` on some
  case-insensitive file systems.

  Also, the :class:`~email.mime.message.MIMEMessage` class now represents any
  kind of MIME message
  with main type :mimetype:`message`.  It takes an optional argument *_subtype*
  which is used to set the MIME subtype.  *_subtype* defaults to
  :mimetype:`rfc822`.

:mod:`mimelib` provided some utility functions in its :mod:`address` and
:mod:`date` modules.  All of these functions have been moved to the
:mod:`email.utils` module.

The ``MsgReader`` class/module has been removed.  Its functionality is most
closely supported in the :func:`~email.iterators.body_line_iterator` function
in the :mod:`email.iterators` module.

.. rubric:: Footnotes

.. [#] Delivery Status Notifications (DSN) are defined in :rfc:`1894`.
