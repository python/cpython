:mod:`email`: Policy Objects
----------------------------

.. module:: email.policy
   :synopsis: Controlling the parsing and generating of messages

.. versionadded: 3.3


The :mod:`email` package's prime focus is the handling of email messages as
described by the various email and MIME RFCs.  However, the general format of
email messages (a block of header fields each consisting of a name followed by
a colon followed by a value, the whole block followed by a blank line and an
arbitrary 'body'), is a format that has found utility outside of the realm of
email.  Some of these uses conform fairly closely to the main RFCs, some do
not.  And even when working with email, there are times when it is desirable to
break strict compliance with the RFCs.

Policy objects give the email package the flexibility to handle all these
disparate use cases.

A :class:`Policy` object encapsulates a set of attributes and methods that
control the behavior of various components of the email package during use.
:class:`Policy` instances can be passed to various classes and methods in the
email package to alter the default behavior.  The settable values and their
defaults are described below.  The :mod:`policy` module also provides some
pre-created :class:`Policy` instances.  In addition to a :const:`default`
instance, there are instances tailored for certain applications.  For example
there is an :const:`SMTP` :class:`Policy` with defaults appropriate for
generating output to be sent to an SMTP server.  These are listed `below
<Policy Instances>`.

In general an application will only need to deal with setting the policy at the
input and output boundaries.  Once parsed, a message is represented by a
:class:`~email.message.Message` object, which is designed to be independent of
the format that the message has "on the wire" when it is received, transmitted,
or displayed.  Thus, a :class:`Policy` can be specified when parsing a message
to create a :class:`~email.message.Message`, and again when turning the
:class:`~email.message.Message` into some other representation.  While often a
program will use the same :class:`Policy` for both input and output, the two
can be different.

As an example, the following code could be used to read an email message from a
file on disk and pass it to the system ``sendmail`` program on a Unix system::

   >>> from email import msg_from_binary_file
   >>> from email.generator import BytesGenerator
   >>> import email.policy
   >>> from subprocess import Popen, PIPE
   >>> with open('mymsg.txt', 'b') as f:
   ...     Msg = msg_from_binary_file(f, policy=email.policy.mbox)
   >>> p = Popen(['sendmail', msg['To'][0].address], stdin=PIPE)
   >>> g = BytesGenerator(p.stdin, policy=email.policy.SMTP)
   >>> g.flatten(msg)
   >>> p.stdin.close()
   >>> rc = p.wait()

Some email package methods accept a *policy* keyword argument, allowing the
policy to be overridden for that method.  For example, the following code uses
the :meth:`email.message.Message.as_string` method of the *msg* object from the
previous example and re-write it to a file using the native line separators for
the platform on which it is running::

   >>> import os
   >>> mypolicy = email.policy.Policy(linesep=os.linesep)
   >>> with open('converted.txt', 'wb') as f:
   ...     f.write(msg.as_string(policy=mypolicy))

Policy instances are immutable, but they can be cloned, accepting the same
keyword arguments as the class constructor and returning a new :class:`Policy`
instance that is a copy of the original but with the specified attributes
values changed.  For example, the following creates an SMTP policy that will
raise any defects detected as errors::

   >>> strict_SMTP = email.policy.SMTP.clone(raise_on_defect=True)

Policy objects can also be combined using the addition operator, producing a
policy object whose settings are a combination of the non-default values of the
summed objects::

   >>> strict_SMTP = email.policy.SMTP + email.policy.strict

This operation is not commutative; that is, the order in which the objects are
added matters.  To illustrate::

   >>> Policy = email.policy.Policy
   >>> apolicy = Policy(max_line_length=100) + Policy(max_line_length=80)
   >>> apolicy.max_line_length
   80
   >>> apolicy = Policy(max_line_length=80) + Policy(max_line_length=100)
   >>> apolicy.max_line_length
   100


.. class:: Policy(**kw)

   The valid constructor keyword arguments are any of the attributes listed
   below.

   .. attribute:: max_line_length

      The maximum length of any line in the serialized output, not counting the
      end of line character(s).  Default is 78, per :rfc:`5322`.  A value of
      ``0`` or :const:`None` indicates that no line wrapping should be
      done at all.

   .. attribute:: linesep

      The string to be used to terminate lines in serialized output.  The
      default is ``\n`` because that's the internal end-of-line discipline used
      by Python, though ``\r\n`` is required by the RFCs.  See `Policy
      Instances`_ for policies that use an RFC conformant linesep.  Setting it
      to :attr:`os.linesep` may also be useful.

   .. attribute:: must_be_7bit

      If ``True``, data output by a bytes generator is limited to ASCII
      characters.  If :const:`False` (the default), then bytes with the high
      bit set are preserved and/or allowed in certain contexts (for example,
      where possible a content transfer encoding of ``8bit`` will be used).
      String generators act as if ``must_be_7bit`` is ``True`` regardless of
      the policy in effect, since a string cannot represent non-ASCII bytes.

   .. attribute:: raise_on_defect

      If :const:`True`, any defects encountered will be raised as errors.  If
      :const:`False` (the default), defects will be passed to the
      :meth:`register_defect` method.

   :mod:`Policy` object also have the following methods:

   .. method:: handle_defect(obj, defect)

      *obj* is the object on which to register the defect.  *defect* should be
      an instance of a  subclass of :class:`~email.errors.Defect`.
      If :attr:`raise_on_defect`
      is ``True`` the defect is raised as an exception.  Otherwise *obj* and
      *defect* are passed to :meth:`register_defect`.  This method is intended
      to be called by parsers when they encounter defects, and will not be
      called by code that uses the email library unless that code is
      implementing an alternate parser.

   .. method:: register_defect(obj, defect)

      *obj* is the object on which to register the defect.  *defect* should be
      a subclass of :class:`~email.errors.Defect`.  This method is part of the
      public API so that custom ``Policy`` subclasses can implement alternate
      handling of defects.  The default implementation calls the ``append``
      method of the ``defects`` attribute of *obj*.

   .. method:: clone(obj, *kw)

      Return a new :class:`Policy` instance whose attributes have the same
      values as the current instance, except where those attributes are
      given new values by the keyword arguments.


Policy Instances
^^^^^^^^^^^^^^^^

The following instances of :class:`Policy` provide defaults suitable for
specific common application domains.

.. data:: default

   An instance of :class:`Policy` with all defaults unchanged.

.. data:: SMTP

   Output serialized from a message will conform to the email and SMTP
   RFCs.  The only changed attribute is :attr:`linesep`, which is set to
   ``\r\n``.

.. data:: HTTP

   Suitable for use when serializing headers for use in HTTP traffic.
   :attr:`linesep` is set to ``\r\n``, and :attr:`max_line_length` is set to
   :const:`None` (unlimited).

.. data:: strict

   :attr:`raise_on_defect` is set to :const:`True`.
