:mod:`!email.saslprep` --- SASLprep string preparation
=======================================================

.. module:: email.saslprep
   :synopsis: RFC 4013 SASLprep string preparation for authentication credentials.

**Source code:** :source:`Lib/email/_saslprep.py`

--------------

.. function:: saslprep(data, *, allow_unassigned_code_points)

   Prepare a Unicode string according to :rfc:`4013` (SASLprep), which is a
   profile of the :rfc:`3454` *stringprep* algorithm.  SASLprep is used to
   normalise usernames and passwords before they are transmitted in
   authentication protocols such as SASL (e.g. SMTP, IMAP, LDAP).

   *data* may be a :class:`str` or :class:`bytes`.  Byte strings are returned
   unchanged.  Unicode strings are processed in four steps:

   1. **Map** — non-ASCII space characters (table C.1.2) are replaced with
      ``U+0020``; characters commonly mapped to nothing (table B.1) are
      removed.
   2. **Normalise** — the string is normalised using Unicode NFKC.
   3. **Prohibit** — a :exc:`ValueError` is raised if the string contains
      any character from the RFC 4013 prohibited-output tables (control
      characters, private-use characters, non-characters, and others).
   4. **Bidi check** — a :exc:`ValueError` is raised if the string mixes
      right-to-left and left-to-right text in a way that violates
      :rfc:`3454` section 6.

   *allow_unassigned_code_points* must be supplied as a keyword argument.
   Pass ``False`` for *stored strings* such as passwords stored in a
   database (unassigned code points are prohibited, per :rfc:`3454`
   section 7).  Pass ``True`` for *queries* such as a password typed at a
   prompt (unassigned code points are permitted).  Always pass this
   explicitly; there is no default.

   Returns the prepared :class:`str`, or the original *data* unchanged if
   it is a :class:`bytes` object.

   >>> from email import saslprep
   >>> saslprep("I\u00ADX", allow_unassigned_code_points=False)  # soft hyphen removed
   'IX'
   >>> saslprep("\u2168", allow_unassigned_code_points=False)     # Roman numeral IX
   'IX'
   >>> saslprep(b"user", allow_unassigned_code_points=False)      # bytes unchanged
   b'user'

   .. versionadded:: 3.15

   .. seealso::

      :rfc:`4013`
         SASLprep: Stringprep Profile for User Names and Passwords.

      :rfc:`3454`
         Preparation of Internationalized Strings ("stringprep").

      :mod:`stringprep`
         The underlying Unicode character tables used by this function.

      :meth:`smtplib.SMTP.login`
         Uses :func:`saslprep` when authenticating with Unicode credentials.
