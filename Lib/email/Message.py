# Copyright (C) 2001 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Basic message object for the email package object model.
"""

from __future__ import generators

import re
import base64
import quopri
from cStringIO import StringIO
from types import ListType

# Intrapackage imports
import Errors
import Utils

SEMISPACE = '; '
paramre = re.compile(r';\s*')



class Message:
    """Basic message object for use inside the object tree.

    A message object is defined as something that has a bunch of RFC 2822
    headers and a payload.  If the body of the message is a multipart, then
    the payload is a list of Messages, otherwise it is a string.

    These objects implement part of the `mapping' interface, which assumes
    there is exactly one occurrance of the header per message.  Some headers
    do in fact appear multiple times (e.g. Received:) and for those headers,
    you must use the explicit API to set or get all the headers.  Not all of
    the mapping methods are implemented.

    """
    def __init__(self):
        self._headers = []
        self._unixfrom = None
        self._payload = None
        # Defaults for multipart messages
        self.preamble = self.epilogue = None

    def __str__(self):
        """Return the entire formatted message as a string.
        This includes the headers, body, and `unixfrom' line.
        """
        return self.as_string(unixfrom=1)

    def as_string(self, unixfrom=0):
        """Return the entire formatted message as a string.
        Optional `unixfrom' when true, means include the Unix From_ envelope
        header.
        """
        from Generator import Generator
        fp = StringIO()
        g = Generator(fp)
        g(self, unixfrom=unixfrom)
        return fp.getvalue()

    def is_multipart(self):
        """Return true if the message consists of multiple parts."""
        if type(self._payload) is ListType:
            return 1
        return 0

    #
    # Unix From_ line
    #
    def set_unixfrom(self, unixfrom):
        self._unixfrom = unixfrom

    def get_unixfrom(self):
        return self._unixfrom

    #
    # Payload manipulation.
    #
    def add_payload(self, payload):
        """Add the given payload to the current payload.

        If the current payload is empty, then the current payload will be made
        a scalar, set to the given value.
        """
        if self._payload is None:
            self._payload = payload
        elif type(self._payload) is ListType:
            self._payload.append(payload)
        elif self.get_main_type() not in (None, 'multipart'):
            raise Errors.MultipartConversionError(
                'Message main Content-Type: must be "multipart" or missing')
        else:
            self._payload = [self._payload, payload]

    # A useful synonym
    attach = add_payload

    def get_payload(self, i=None, decode=0):
        """Return the current payload exactly as is.

        Optional i returns that index into the payload.

        Optional decode is a flag indicating whether the payload should be
        decoded or not, according to the Content-Transfer-Encoding: header.
        When true and the message is not a multipart, the payload will be
        decoded if this header's value is `quoted-printable' or `base64'.  If
        some other encoding is used, or the header is missing, the payload is
        returned as-is (undecoded).  If the message is a multipart and the
        decode flag is true, then None is returned.
        """
        if i is None:
            payload = self._payload
        elif type(self._payload) is not ListType:
            raise TypeError, i
        else:
            payload = self._payload[i]
        if decode:
            if self.is_multipart():
                return None
            cte = self.get('content-transfer-encoding', '')
            if cte.lower() == 'quoted-printable':
                return Utils._qdecode(payload)
            elif cte.lower() == 'base64':
                return Utils._bdecode(payload)
        # Everything else, including encodings with 8bit or 7bit are returned
        # unchanged.
        return payload


    def set_payload(self, payload):
        """Set the payload to the given value."""
        self._payload = payload

    #
    # MAPPING INTERFACE (partial)
    #
    def __len__(self):
        """Return the total number of headers, including duplicates."""
        return len(self._headers)

    def __getitem__(self, name):
        """Get a header value.

        Return None if the header is missing instead of raising an exception.

        Note that if the header appeared multiple times, exactly which
        occurrance gets returned is undefined.  Use getall() to get all
        the values matching a header field name.
        """
        return self.get(name)

    def __setitem__(self, name, val):
        """Set the value of a header.

        Note: this does not overwrite an existing header with the same field
        name.  Use __delitem__() first to delete any existing headers.
        """
        self._headers.append((name, val))

    def __delitem__(self, name):
        """Delete all occurrences of a header, if present.

        Does not raise an exception if the header is missing.
        """
        name = name.lower()
        newheaders = []
        for k, v in self._headers:
            if k.lower() <> name:
                newheaders.append((k, v))
        self._headers = newheaders

    def __contains__(self, key):
        return key.lower() in [k.lower() for k, v in self._headers]

    def has_key(self, name):
        """Return true if the message contains the header."""
        missing = []
        return self.get(name, missing) is not missing

    def keys(self):
        """Return a list of all the message's header field names.

        These will be sorted in the order they appeared in the original
        message, and may contain duplicates.  Any fields deleted and
        re-inserted are always appended to the header list.
        """
        return [k for k, v in self._headers]

    def values(self):
        """Return a list of all the message's header values.

        These will be sorted in the order they appeared in the original
        message, and may contain duplicates.  Any fields deleted and
        re-inserted are alwyas appended to the header list.
        """
        return [v for k, v in self._headers]

    def items(self):
        """Get all the message's header fields and values.

        These will be sorted in the order they appeared in the original
        message, and may contain duplicates.  Any fields deleted and
        re-inserted are alwyas appended to the header list.
        """
        return self._headers[:]

    def get(self, name, failobj=None):
        """Get a header value.

        Like __getitem__() but return failobj instead of None when the field
        is missing.
        """
        name = name.lower()
        for k, v in self._headers:
            if k.lower() == name:
                return v
        return failobj

    #
    # Additional useful stuff
    #

    def get_all(self, name, failobj=None):
        """Return a list of all the values for the named field.

        These will be sorted in the order they appeared in the original
        message, and may contain duplicates.  Any fields deleted and
        re-inserted are alwyas appended to the header list.

        If no such fields exist, failobj is returned (defaults to None).
        """
        values = []
        name = name.lower()
        for k, v in self._headers:
            if k.lower() == name:
                values.append(v)
        if not values:
            return failobj
        return values

    def add_header(self, _name, _value, **_params):
        """Extended header setting.

        name is the header field to add.  keyword arguments can be used to set
        additional parameters for the header field, with underscores converted
        to dashes.  Normally the parameter will be added as key="value" unless
        value is None, in which case only the key will be added.

        Example:

        msg.add_header('content-disposition', 'attachment', filename='bud.gif')

        """
        parts = []
        for k, v in _params.items():
            if v is None:
                parts.append(k.replace('_', '-'))
            else:
                parts.append('%s="%s"' % (k.replace('_', '-'), v))
        if _value is not None:
            parts.insert(0, _value)
        self._headers.append((_name, SEMISPACE.join(parts)))

    def get_type(self, failobj=None):
        """Returns the message's content type.

        The returned string is coerced to lowercase and returned as a single
        string of the form `maintype/subtype'.  If there was no Content-Type:
        header in the message, failobj is returned (defaults to None).
        """
        missing = []
        value = self.get('content-type', missing)
        if value is missing:
            return failobj
        return paramre.split(value)[0].lower()

    def get_main_type(self, failobj=None):
        """Return the message's main content type if present."""
        missing = []
        ctype = self.get_type(missing)
        if ctype is missing:
            return failobj
        parts = ctype.split('/')
        if len(parts) > 0:
            return ctype.split('/')[0]
        return failobj

    def get_subtype(self, failobj=None):
        """Return the message's content subtype if present."""
        missing = []
        ctype = self.get_type(missing)
        if ctype is missing:
            return failobj
        parts = ctype.split('/')
        if len(parts) > 1:
            return ctype.split('/')[1]
        return failobj

    def _get_params_preserve(self, failobj, header):
        # Like get_params() but preserves the quoting of values.  BAW:
        # should this be part of the public interface?
        missing = []
        value = self.get(header, missing)
        if value is missing:
            return failobj
        params = []
        for p in paramre.split(value):
            try:
                name, val = p.split('=', 1)
            except ValueError:
                # Must have been a bare attribute
                name = p
                val = ''
            params.append((name, val))
        return params

    def get_params(self, failobj=None, header='content-type'):
        """Return the message's Content-Type: parameters, as a list.

        The elements of the returned list are 2-tuples of key/value pairs, as
        split on the `=' sign.  The left hand side of the `=' is the key,
        while the right hand side is the value.  If there is no `=' sign in
        the parameter the value is the empty string.  The value is always
        unquoted.

        Optional failobj is the object to return if there is no Content-Type:
        header.  Optional header is the header to search instead of
        Content-Type:
        """
        missing = []
        params = self._get_params_preserve(missing, header)
        if params is missing:
            return failobj
        return [(k, Utils.unquote(v)) for k, v in params]

    def get_param(self, param, failobj=None, header='content-type'):
        """Return the parameter value if found in the Content-Type: header.

        Optional failobj is the object to return if there is no Content-Type:
        header.  Optional header is the header to search instead of
        Content-Type:

        Parameter keys are always compared case insensitively.  Values are
        always unquoted.
        """
        if not self.has_key(header):
            return failobj
        for k, v in self._get_params_preserve(failobj, header):
            if k.lower() == param.lower():
                return Utils.unquote(v)
        return failobj

    def get_filename(self, failobj=None):
        """Return the filename associated with the payload if present.

        The filename is extracted from the Content-Disposition: header's
        `filename' parameter, and it is unquoted.
        """
        missing = []
        filename = self.get_param('filename', missing, 'content-disposition')
        if filename is missing:
            return failobj
        return Utils.unquote(filename.strip())

    def get_boundary(self, failobj=None):
        """Return the boundary associated with the payload if present.

        The boundary is extracted from the Content-Type: header's `boundary'
        parameter, and it is unquoted.
        """
        missing = []
        boundary = self.get_param('boundary', missing)
        if boundary is missing:
            return failobj
        return Utils.unquote(boundary.strip())

    def set_boundary(self, boundary):
        """Set the boundary parameter in Content-Type: to 'boundary'.

        This is subtly different than deleting the Content-Type: header and
        adding a new one with a new boundary parameter via add_header().  The
        main difference is that using the set_boundary() method preserves the
        order of the Content-Type: header in the original message.

        HeaderParseError is raised if the message has no Content-Type: header.
        """
        missing = []
        params = self._get_params_preserve(missing, 'content-type')
        if params is missing:
            # There was no Content-Type: header, and we don't know what type
            # to set it to, so raise an exception.
            raise Errors.HeaderParseError, 'No Content-Type: header found'
        newparams = []
        foundp = 0
        for pk, pv in params:
            if pk.lower() == 'boundary':
                newparams.append(('boundary', '"%s"' % boundary))
                foundp = 1
            else:
                newparams.append((pk, pv))
        if not foundp:
            # The original Content-Type: header had no boundary attribute.
            # Tack one one the end.  BAW: should we raise an exception
            # instead???
            newparams.append(('boundary', '"%s"' % boundary))
        # Replace the existing Content-Type: header with the new value
        newheaders = []
        for h, v in self._headers:
            if h.lower() == 'content-type':
                parts = []
                for k, v in newparams:
                    if v == '':
                        parts.append(k)
                    else:
                        parts.append('%s=%s' % (k, v))
                newheaders.append((h, SEMISPACE.join(parts)))

            else:
                newheaders.append((h, v))
        self._headers = newheaders

    def walk(self):
        """Walk over the message tree, yielding each subpart.

        The walk is performed in breadth-first order.  This method is a
        generator.
        """
        yield self
        if self.is_multipart():
            for subpart in self.get_payload():
                for subsubpart in subpart.walk():
                    yield subsubpart

    def get_charsets(self, failobj=None):
        """Return a list containing the charset(s) used in this message.

        The returned list of items describes the Content-Type: headers'
        charset parameter for this message and all the subparts in its
        payload.

        Each item will either be a string (the value of the charset parameter
        in the Content-Type: header of that part) or the value of the
        'failobj' parameter (defaults to None), if the part does not have a
        main MIME type of "text", or the charset is not defined.

        The list will contain one string for each part of the message, plus
        one for the container message (i.e. self), so that a non-multipart
        message will still return a list of length 1.
        """
        return [part.get_param('charset', failobj) for part in self.walk()]
