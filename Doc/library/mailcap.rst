:mod:`mailcap` --- Mailcap file handling
========================================

.. module:: mailcap
   :synopsis: Mailcap file handling.
   :deprecated:

**Source code:** :source:`Lib/mailcap.py`

.. deprecated-removed:: 3.11 3.13
   The :mod:`mailcap` module is deprecated
   (see :pep:`PEP 594 <594#mailcap>` for details).
   The :mod:`mimetypes` module provides an alternative.

--------------

Mailcap files are used to configure how MIME-aware applications such as mail
readers and web browsers react to files with different MIME types. (The name
"mailcap" is derived from the phrase "mail capability".)  For example, a mailcap
file might contain a line like ``video/mpeg; xmpeg %s``.  Then, if the user
encounters an email message or web document with the MIME type
:mimetype:`video/mpeg`, ``%s`` will be replaced by a filename (usually one
belonging to a temporary file) and the :program:`xmpeg` program can be
automatically started to view the file.

The mailcap format is documented in :rfc:`1524`, "A User Agent Configuration
Mechanism For Multimedia Mail Format Information", but is not an internet
standard.  However, mailcap files are supported on most Unix systems.


.. function:: findmatch(caps, MIMEtype, key='view', filename='/dev/null', plist=[])

   Return a 2-tuple; the first element is a string containing the command line to
   be executed (which can be passed to :func:`os.system`), and the second element
   is the mailcap entry for a given MIME type.  If no matching MIME type can be
   found, ``(None, None)`` is returned.

   *key* is the name of the field desired, which represents the type of activity to
   be performed; the default value is 'view', since in the  most common case you
   simply want to view the body of the MIME-typed data.  Other possible values
   might be 'compose' and 'edit', if you wanted to create a new body of the given
   MIME type or alter the existing body data.  See :rfc:`1524` for a complete list
   of these fields.

   *filename* is the filename to be substituted for ``%s`` in the command line; the
   default value is ``'/dev/null'`` which is almost certainly not what you want, so
   usually you'll override it by specifying a filename.

   *plist* can be a list containing named parameters; the default value is simply
   an empty list.  Each entry in the list must be a string containing the parameter
   name, an equals sign (``'='``), and the parameter's value.  Mailcap entries can
   contain  named parameters like ``%{foo}``, which will be replaced by the value
   of the parameter named 'foo'.  For example, if the command line ``showpartial
   %{id} %{number} %{total}`` was in a mailcap file, and *plist* was set to
   ``['id=1', 'number=2', 'total=3']``, the resulting command line would be
   ``'showpartial 1 2 3'``.

   In a mailcap file, the "test" field can optionally be specified to test some
   external condition (such as the machine architecture, or the window system in
   use) to determine whether or not the mailcap line applies.  :func:`findmatch`
   will automatically check such conditions and skip the entry if the check fails.

   .. versionchanged:: 3.11

      To prevent security issues with shell metacharacters (symbols that have
      special effects in a shell command line), ``findmatch`` will refuse
      to inject ASCII characters other than alphanumerics and ``@+=:,./-_``
      into the returned command line.

      If a disallowed character appears in *filename*, ``findmatch`` will always
      return ``(None, None)`` as if no entry was found.
      If such a character appears elsewhere (a value in *plist* or in *MIMEtype*),
      ``findmatch`` will ignore all mailcap entries which use that value.
      A :mod:`warning <warnings>` will be raised in either case.

.. function:: getcaps()

   Returns a dictionary mapping MIME types to a list of mailcap file entries. This
   dictionary must be passed to the :func:`findmatch` function.  An entry is stored
   as a list of dictionaries, but it shouldn't be necessary to know the details of
   this representation.

   The information is derived from all of the mailcap files found on the system.
   Settings in the user's mailcap file :file:`$HOME/.mailcap` will override
   settings in the system mailcap files :file:`/etc/mailcap`,
   :file:`/usr/etc/mailcap`, and :file:`/usr/local/etc/mailcap`.

An example usage::

   >>> import mailcap
   >>> d = mailcap.getcaps()
   >>> mailcap.findmatch(d, 'video/mpeg', filename='tmp1223')
   ('xmpeg tmp1223', {'view': 'xmpeg %s'})

