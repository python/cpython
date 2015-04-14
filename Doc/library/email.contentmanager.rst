:mod:`email.contentmanager`: Managing MIME Content
--------------------------------------------------

.. module:: email.contentmanager
   :synopsis: Storing and Retrieving Content from MIME Parts

.. moduleauthor:: R. David Murray <rdmurray@bitdance.com>
.. sectionauthor:: R. David Murray <rdmurray@bitdance.com>


.. note::

   The contentmanager module has been included in the standard library on a
   :term:`provisional basis <provisional package>`. Backwards incompatible
   changes (up to and including removal of the module) may occur if deemed
   necessary by the core developers.

.. versionadded:: 3.4
   as a :term:`provisional module <provisional package>`.

The :mod:`~email.message` module provides a class that can represent an
arbitrary email message.  That basic message model has a useful and flexible
API, but it provides only a lower-level API for interacting with the generic
parts of a message (the headers, generic header parameters, and the payload,
which may be a list of sub-parts).  This module provides classes and tools
that provide an enhanced and extensible API for dealing with various specific
types of content, including the ability to retrieve the content of the message
as a specialized object type rather than as a simple bytes object.  The module
automatically takes care of the RFC-specified MIME details (required headers
and parameters, etc.) for the certain common content types content properties,
and support for additional types can be added by an application using the
extension mechanisms.

This module defines the eponymous "Content Manager" classes.  The base
:class:`.ContentManager` class defines an API for registering content
management functions which extract data from ``Message`` objects or insert data
and headers into ``Message`` objects, thus providing a way of converting
between ``Message`` objects containing data and other representations of that
data (Python data types, specialized Python objects, external files, etc).  The
module also defines one concrete content manager: :data:`raw_data_manager`
converts between MIME content types and ``str`` or ``bytes`` data.  It also
provides a convenient API for managing the MIME parameters when inserting
content into ``Message``\ s.  It also handles inserting and extracting
``Message`` objects when dealing with the ``message/rfc822`` content type.

Another part of the enhanced interface is subclasses of
:class:`~email.message.Message` that provide new convenience API functions,
including convenience methods for calling the Content Managers derived from
this module.

.. note::

   Although :class:`.EmailMessage` and :class:`.MIMEPart` are currently
   documented in this module because of the provisional nature of the code, the
   implementation lives in the :mod:`email.message` module.

.. currentmodule:: email.message

.. class:: EmailMessage(policy=default)

   If *policy* is specified (it must be an instance of a :mod:`~email.policy`
   class) use the rules it specifies to udpate and serialize the representation
   of the message.  If *policy* is not set, use the
   :class:`~email.policy.default` policy, which follows the rules of the email
   RFCs except for line endings (instead of the RFC mandated ``\r\n``, it uses
   the Python standard ``\n`` line endings).  For more information see the
   :mod:`~email.policy` documentation.

   This class is a subclass of :class:`~email.message.Message`.  It adds
   the following methods:


   .. method:: is_attachment

      Return ``True`` if there is a :mailheader:`Content-Disposition` header
      and its (case insensitive) value is ``attachment``, ``False`` otherwise.

      .. versionchanged:: 3.4.2
         is_attachment is now a method instead of a property, for consistency
         with :meth:`~email.message.Message.is_multipart`.


   .. method:: get_body(preferencelist=('related', 'html', 'plain'))

      Return the MIME part that is the best candidate to be the "body" of the
      message.

      *preferencelist* must be a sequence of strings from the set ``related``,
      ``html``, and ``plain``, and indicates the order of preference for the
      content type of the part returned.

      Start looking for candidate matches with the object on which the
      ``get_body`` method is called.

      If ``related`` is not included in *preferencelist*, consider the root
      part (or subpart of the root part) of any related encountered as a
      candidate if the (sub-)part matches a preference.

      When encountering a ``multipart/related``, check the ``start`` parameter
      and if a part with a matching :mailheader:`Content-ID` is found, consider
      only it when looking for candidate matches.  Otherwise consider only the
      first (default root) part of the ``multipart/related``.

      If a part has a :mailheader:`Content-Disposition` header, only consider
      the part a candidate match if the value of the header is ``inline``.

      If none of the candidates matches any of the preferences in
      *preferneclist*, return ``None``.

      Notes: (1) For most applications the only *preferencelist* combinations
      that really make sense are ``('plain',)``, ``('html', 'plain')``, and the
      default, ``('related', 'html', 'plain')``.  (2) Because matching starts
      with the object on which ``get_body`` is called, calling ``get_body`` on
      a ``multipart/related`` will return the object itself unless
      *preferencelist* has a non-default value. (3) Messages (or message parts)
      that do not specify a :mailheader:`Content-Type` or whose
      :mailheader:`Content-Type` header is invalid will be treated as if they
      are of type ``text/plain``, which may occasionally cause ``get_body`` to
      return unexpected results.


   .. method:: iter_attachments()

      Return an iterator over all of the parts of the message that are not
      candidate "body" parts.  That is, skip the first occurrence of each of
      ``text/plain``, ``text/html``, ``multipart/related``, or
      ``multipart/alternative`` (unless they are explicitly marked as
      attachments via :mailheader:`Content-Disposition: attachment`), and
      return all remaining parts.  When applied directly to a
      ``multipart/related``, return an iterator over the all the related parts
      except the root part (ie: the part pointed to by the ``start`` parameter,
      or the first part if there is no ``start`` parameter or the ``start``
      parameter doesn't match the :mailheader:`Content-ID` of any of the
      parts).  When applied directly to a ``multipart/alternative`` or a
      non-``multipart``, return an empty iterator.


   .. method:: iter_parts()

      Return an iterator over all of the immediate sub-parts of the message,
      which will be empty for a non-``multipart``.  (See also
      :meth:`~email.message.walk`.)


   .. method:: get_content(*args, content_manager=None, **kw)

      Call the ``get_content`` method of the *content_manager*, passing self
      as the message object, and passing along any other arguments or keywords
      as additional arguments.  If *content_manager* is not specified, use
      the ``content_manager`` specified by the current :mod:`~email.policy`.


   .. method:: set_content(*args, content_manager=None, **kw)

      Call the ``set_content`` method of the *content_manager*, passing self
      as the message object, and passing along any other arguments or keywords
      as additional arguments.  If *content_manager* is not specified, use
      the ``content_manager`` specified by the current :mod:`~email.policy`.


   .. method:: make_related(boundary=None)

      Convert a non-``multipart`` message into a ``multipart/related`` message,
      moving any existing :mailheader:`Content-` headers and payload into a
      (new) first part of the ``multipart``.  If *boundary* is specified, use
      it as the boundary string in the multipart, otherwise leave the boundary
      to be automatically created when it is needed (for example, when the
      message is serialized).


   .. method:: make_alternative(boundary=None)

      Convert a non-``multipart`` or a ``multipart/related`` into a
      ``multipart/alternative``, moving any existing :mailheader:`Content-`
      headers and payload into a (new) first part of the ``multipart``.  If
      *boundary* is specified, use it as the boundary string in the multipart,
      otherwise leave the boundary to be automatically created when it is
      needed (for example, when the message is serialized).


   .. method:: make_mixed(boundary=None)

      Convert a non-``multipart``, a ``multipart/related``, or a
      ``multipart-alternative`` into a ``multipart/mixed``, moving any existing
      :mailheader:`Content-` headers and payload into a (new) first part of the
      ``multipart``.  If *boundary* is specified, use it as the boundary string
      in the multipart, otherwise leave the boundary to be automatically
      created when it is needed (for example, when the message is serialized).


   .. method:: add_related(*args, content_manager=None, **kw)

      If the message is a ``multipart/related``, create a new message
      object, pass all of the arguments to its :meth:`set_content` method,
      and :meth:`~email.message.Message.attach` it to the ``multipart``.  If
      the message is a non-``multipart``, call :meth:`make_related` and then
      proceed as above.  If the message is any other type of ``multipart``,
      raise a :exc:`TypeError`. If *content_manager* is not specified, use
      the ``content_manager`` specified by the current :mod:`~email.policy`.
      If the added part has no :mailheader:`Content-Disposition` header,
      add one with the value ``inline``.


   .. method:: add_alternative(*args, content_manager=None, **kw)

      If the message is a ``multipart/alternative``, create a new message
      object, pass all of the arguments to its :meth:`set_content` method, and
      :meth:`~email.message.Message.attach` it to the ``multipart``.  If the
      message is a non-``multipart`` or ``multipart/related``, call
      :meth:`make_alternative` and then proceed as above.  If the message is
      any other type of ``multipart``, raise a :exc:`TypeError`. If
      *content_manager* is not specified, use the ``content_manager`` specified
      by the current :mod:`~email.policy`.


   .. method:: add_attachment(*args, content_manager=None, **kw)

      If the message is a ``multipart/mixed``, create a new message object,
      pass all of the arguments to its :meth:`set_content` method, and
      :meth:`~email.message.Message.attach` it to the ``multipart``.  If the
      message is a non-``multipart``, ``multipart/related``, or
      ``multipart/alternative``, call :meth:`make_mixed` and then proceed as
      above. If *content_manager* is not specified, use the ``content_manager``
      specified by the current :mod:`~email.policy`.  If the added part
      has no :mailheader:`Content-Disposition` header, add one with the value
      ``attachment``.  This method can be used both for explicit attachments
      (:mailheader:`Content-Disposition: attachment` and ``inline`` attachments
      (:mailheader:`Content-Disposition: inline`), by passing appropriate
      options to the ``content_manager``.


   .. method:: clear()

      Remove the payload and all of the headers.


   .. method:: clear_content()

      Remove the payload and all of the :exc:`Content-` headers, leaving
      all other headers intact and in their original order.


.. class:: MIMEPart(policy=default)

    This class represents a subpart of a MIME message.  It is identical to
    :class:`EmailMessage`, except that no :mailheader:`MIME-Version` headers are
    added when :meth:`~EmailMessage.set_content` is called, since sub-parts do
    not need their own :mailheader:`MIME-Version` headers.


.. currentmodule:: email.contentmanager

.. class:: ContentManager()

   Base class for content managers.  Provides the standard registry mechanisms
   to register converters between MIME content and other representations, as
   well as the ``get_content`` and ``set_content`` dispatch methods.


   .. method:: get_content(msg, *args, **kw)

      Look up a handler function based on the ``mimetype`` of *msg* (see next
      paragraph), call it, passing through all arguments, and return the result
      of the call.  The expectation is that the handler will extract the
      payload from *msg* and return an object that encodes information about
      the extracted data.

      To find the handler, look for the following keys in the registry,
      stopping with the first one found:

            * the string representing the full MIME type (``maintype/subtype``)
            * the string representing the ``maintype``
            * the empty string

      If none of these keys produce a handler, raise a :exc:`KeyError` for the
      full MIME type.


   .. method:: set_content(msg, obj, *args, **kw)

      If the ``maintype`` is ``multipart``, raise a :exc:`TypeError`; otherwise
      look up a handler function based on the type of *obj* (see next
      paragraph), call :meth:`~email.message.EmailMessage.clear_content` on the
      *msg*, and call the handler function, passing through all arguments.  The
      expectation is that the handler will transform and store *obj* into
      *msg*, possibly making other changes to *msg* as well, such as adding
      various MIME headers to encode information needed to interpret the stored
      data.

      To find the handler, obtain the type of *obj* (``typ = type(obj)``), and
      look for the following keys in the registry, stopping with the first one
      found:

           * the type itself (``typ``)
           * the type's fully qualified name (``typ.__module__ + '.' +
             typ.__qualname__``).
           * the type's qualname (``typ.__qualname__``)
           * the type's name (``typ.__name__``).

      If none of the above match, repeat all of the checks above for each of
      the types in the :term:`MRO` (``typ.__mro__``).  Finally, if no other key
      yields a handler, check for a handler for the key ``None``.  If there is
      no handler for ``None``, raise a :exc:`KeyError` for the fully
      qualified name of the type.

      Also add a :mailheader:`MIME-Version` header if one is not present (see
      also :class:`.MIMEPart`).


   .. method:: add_get_handler(key, handler)

      Record the function *handler* as the handler for *key*.  For the possible
      values of *key*, see :meth:`get_content`.


   .. method:: add_set_handler(typekey, handler)

      Record *handler* as the function to call when an object of a type
      matching *typekey* is passed to :meth:`set_content`.  For the possible
      values of *typekey*, see :meth:`set_content`.


Content Manager Instances
~~~~~~~~~~~~~~~~~~~~~~~~~

Currently the email package provides only one concrete content manager,
:data:`raw_data_manager`, although more may be added in the future.
:data:`raw_data_manager` is the
:attr:`~email.policy.EmailPolicy.content_manager` provided by
:attr:`~email.policy.EmailPolicy` and its derivatives.


.. data:: raw_data_manager

   This content manager provides only a minimum interface beyond that provided
   by :class:`~email.message.Message` itself:  it deals only with text, raw
   byte strings, and :class:`~email.message.Message` objects.  Nevertheless, it
   provides significant advantages compared to the base API: ``get_content`` on
   a text part will return a unicode string without the application needing to
   manually decode it, ``set_content`` provides a rich set of options for
   controlling the headers added to a part and controlling the content transfer
   encoding, and it enables the use of the various ``add_`` methods, thereby
   simplifying the creation of multipart messages.

   .. method:: get_content(msg, errors='replace')

      Return the payload of the part as either a string (for ``text`` parts), a
      :class:`~email.message.EmailMessage` object (for ``message/rfc822``
      parts), or a ``bytes`` object (for all other non-multipart types).  Raise
      a :exc:`KeyError` if called on a ``multipart``.  If the part is a
      ``text`` part and *errors* is specified, use it as the error handler when
      decoding the payload to unicode.  The default error handler is
      ``replace``.

   .. method:: set_content(msg, <'str'>, subtype="plain", charset='utf-8' \
                           cte=None, \
                           disposition=None, filename=None, cid=None, \
                           params=None, headers=None)
               set_content(msg, <'bytes'>, maintype, subtype, cte="base64", \
                           disposition=None, filename=None, cid=None, \
                           params=None, headers=None)
               set_content(msg, <'Message'>, cte=None, \
                           disposition=None, filename=None, cid=None, \
                           params=None, headers=None)
               set_content(msg, <'list'>, subtype='mixed', \
                           disposition=None, filename=None, cid=None, \
                           params=None, headers=None)

       Add headers and payload to *msg*:

       Add a :mailheader:`Content-Type` header with a ``maintype/subtype``
       value.

           * For ``str``, set the MIME ``maintype`` to ``text``, and set the
             subtype to *subtype* if it is specified, or ``plain`` if it is not.
           * For ``bytes``, use the specified *maintype* and *subtype*, or
             raise a :exc:`TypeError` if they are not specified.
           * For :class:`~email.message.Message` objects, set the maintype to
             ``message``, and set the subtype to *subtype* if it is specified
             or ``rfc822`` if it is not.  If *subtype* is ``partial``, raise an
             error (``bytes`` objects must be used to construct
             ``message/partial`` parts).
           * For *<'list'>*, which should be a list of
             :class:`~email.message.Message` objects, set the ``maintype`` to
             ``multipart``, and the ``subtype`` to *subtype* if it is
             specified, and ``mixed`` if it is not.  If the message parts in
             the *<'list'>* have :mailheader:`MIME-Version` headers, remove
             them.

       If *charset* is provided (which is valid only for ``str``), encode the
       string to bytes using the specified character set.  The default is
       ``utf-8``.  If the specified *charset* is a known alias for a standard
       MIME charset name, use the standard charset instead.

       If *cte* is set, encode the payload using the specified content transfer
       encoding, and set the :mailheader:`Content-Transfer-Endcoding` header to
       that value.  For ``str`` objects, if it is not set use heuristics to
       determine the most compact encoding.  Possible values for *cte* are
       ``quoted-printable``, ``base64``, ``7bit``, ``8bit``, and ``binary``.
       If the input cannot be encoded in the specified encoding (eg: ``7bit``),
       raise a :exc:`ValueError`.  For :class:`~email.message.Message`, per
       :rfc:`2046`, raise an error if a *cte* of ``quoted-printable`` or
       ``base64`` is requested for *subtype* ``rfc822``, and for any *cte*
       other than ``7bit`` for *subtype* ``external-body``.  For
       ``message/rfc822``, use ``8bit`` if *cte* is not specified.  For all
       other values of *subtype*, use ``7bit``.

       .. note:: A *cte* of ``binary`` does not actually work correctly yet.
          The ``Message`` object as modified by ``set_content`` is correct, but
          :class:`~email.generator.BytesGenerator` does not serialize it
          correctly.

       If *disposition* is set, use it as the value of the
       :mailheader:`Content-Disposition` header.  If not specified, and
       *filename* is specified, add the header with the value ``attachment``.
       If it is not specified and *filename* is also not specified, do not add
       the header.  The only valid values for *disposition* are ``attachment``
       and ``inline``.

       If *filename* is specified, use it as the value of the ``filename``
       parameter of the :mailheader:`Content-Disposition` header.  There is no
       default.

       If *cid* is specified, add a :mailheader:`Content-ID` header with
       *cid* as its value.

       If *params* is specified, iterate its ``items`` method and use the
       resulting ``(key, value)`` pairs to set additional parameters on the
       :mailheader:`Content-Type` header.

       If *headers* is specified and is a list of strings of the form
       ``headername: headervalue`` or a list of ``header`` objects
       (distinguised from strings by having a ``name`` attribute), add the
       headers to *msg*.
