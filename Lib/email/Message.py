# Copyright (C) 2001,2002 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Basic message object for the email package object model.
"""

import re
import warnings
from cStringIO import StringIO
from types import ListType, TupleType, StringType

# Intrapackage imports
from email import Errors
from email import Utils
from email import Charset

SEMISPACE = '; '

# Regular expression used to split header parameters.  BAW: this may be too
# simple.  It isn't strictly RFC 2045 (section 5.1) compliant, but it catches
# most headers found in the wild.  We may eventually need a full fledged
# parser eventually.
paramre = re.compile(r'\s*;\s*')
# Regular expression that matches `special' characters in parameters, the
# existance of which force quoting of the parameter value.
tspecials = re.compile(r'[ \(\)<>@,;:\\"/\[\]\?=]')



# Helper functions
def _formatparam(param, value=None, quote=1):
    """Convenience function to format and return a key=value pair.

    This will quote the value if needed or if quote is true.
    """
    if value is not None and len(value) > 0:
        # TupleType is used for RFC 2231 encoded parameter values where items
        # are (charset, language, value).  charset is a string, not a Charset
        # instance.
        if isinstance(value, TupleType):
            # Convert to ascii, ignore language
            value = unicode(value[2], value[0]).encode("ascii")
        # BAW: Please check this.  I think that if quote is set it should
        # force quoting even if not necessary.
        if quote or tspecials.search(value):
            return '%s="%s"' % (param, Utils.quote(value))
        else:
            return '%s=%s' % (param, value)
    else:
        return param


def _unquotevalue(value):
    if isinstance(value, TupleType):
       return (value[0], value[1], Utils.unquote(value[2]))
    else:
       return Utils.unquote(value)



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
        self._charset = None
        # Defaults for multipart messages
        self.preamble = self.epilogue = None
        # Default content type
        self._default_type = 'text/plain'

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
        from email.Generator import Generator
        fp = StringIO()
        g = Generator(fp)
        g.flatten(self, unixfrom=unixfrom)
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
        warnings.warn('add_payload() is deprecated, use attach() instead.',
                      DeprecationWarning, 2)
        if self._payload is None:
            self._payload = payload
        elif type(self._payload) is ListType:
            self._payload.append(payload)
        elif self.get_main_type() not in (None, 'multipart'):
            raise Errors.MultipartConversionError(
                'Message main Content-Type: must be "multipart" or missing')
        else:
            self._payload = [self._payload, payload]

    def attach(self, payload):
        """Add the given payload to the current payload.

        The current payload will always be a list of objects after this method
        is called.  If you want to set the payload to a scalar object
        (e.g. because you're attaching a message/rfc822 subpart), use
        set_payload() instead.
        """
        if self._payload is None:
            self._payload = [payload]
        else:
            self._payload.append(payload)

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


    def set_payload(self, payload, charset=None):
        """Set the payload to the given value.

        Optionally set the charset, which must be a Charset instance."""
        self._payload = payload
        if charset is not None:
            self.set_charset(charset)

    def set_charset(self, charset):
        """Set the charset of the payload to a given character set.

        charset can be a string or a Charset object.  If it is a string, it
        will be converted to a Charset object by calling Charset's
        constructor.  If charset is None, the charset parameter will be
        removed from the Content-Type: field.  Anything else will generate a
        TypeError.

        The message will be assumed to be a text message encoded with
        charset.input_charset.  It will be converted to charset.output_charset
        and encoded properly, if needed, when generating the plain text
        representation of the message.  MIME headers (MIME-Version,
        Content-Type, Content-Transfer-Encoding) will be added as needed.
        """
        if charset is None:
            self.del_param('charset')
            self._charset = None
            return
        if isinstance(charset, StringType):
            charset = Charset.Charset(charset)
        if not isinstance(charset, Charset.Charset):
            raise TypeError, charset
        # BAW: should we accept strings that can serve as arguments to the
        # Charset constructor?
        self._charset = charset
        if not self.has_key('MIME-Version'):
            self.add_header('MIME-Version', '1.0')
        if not self.has_key('Content-Type'):
            self.add_header('Content-Type', 'text/plain',
                            charset=charset.get_output_charset())
        else:
            self.set_param('charset', charset.get_output_charset())
        if not self.has_key('Content-Transfer-Encoding'):
            cte = charset.get_body_encoding()
            if callable(cte):
                cte(self)
            else:
                self.add_header('Content-Transfer-Encoding', cte)

    def get_charset(self):
        """Return the Charset object associated with the message's payload."""
        return self._charset

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
        re-inserted are always appended to the header list.
        """
        return [v for k, v in self._headers]

    def items(self):
        """Get all the message's header fields and values.

        These will be sorted in the order they appeared in the original
        message, and may contain duplicates.  Any fields deleted and
        re-inserted are always appended to the header list.
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
        re-inserted are always appended to the header list.

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
                parts.append(_formatparam(k.replace('_', '-'), v))
        if _value is not None:
            parts.insert(0, _value)
        self._headers.append((_name, SEMISPACE.join(parts)))

    #
    # These methods are silently deprecated in favor of get_content_type() and
    # friends (see below).  They will be noisily deprecated in email 3.0.
    #

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
        return paramre.split(value)[0].lower().strip()

    def get_main_type(self, failobj=None):
        """Return the message's main content type if present."""
        missing = []
        ctype = self.get_type(missing)
        if ctype is missing:
            return failobj
        if ctype.count('/') <> 1:
            return failobj
        return ctype.split('/')[0]

    def get_subtype(self, failobj=None):
        """Return the message's content subtype if present."""
        missing = []
        ctype = self.get_type(missing)
        if ctype is missing:
            return failobj
        if ctype.count('/') <> 1:
            return failobj
        return ctype.split('/')[1]

    #
    # Use these three methods instead of the three above.
    #

    def get_content_type(self):
        """Returns the message's content type.

        The returned string is coerced to lowercase and returned as a ingle
        string of the form `maintype/subtype'.  If there was no Content-Type:
        header in the message, the default type as give by get_default_type()
        will be returned.  Since messages always have a default type this will
        always return a value.

        The current state of RFC standards define a message's default type to
        be text/plain unless it appears inside a multipart/digest container,
        in which case it would be message/rfc822.
        """
        missing = []
        value = self.get('content-type', missing)
        if value is missing:
            # This should have no parameters
            return self.get_default_type()
        ctype = paramre.split(value)[0].lower().strip()
        # RFC 2045, section 5.2 says if its invalid, use text/plain
        if ctype.count('/') <> 1:
            return 'text/plain'
        return ctype

    def get_content_maintype(self):
        """Returns the message's main content type.

        This is the `maintype' part of the string returned by
        get_content_type().  If no slash is found in the full content type, a
        ValueError is raised.
        """
        ctype = self.get_content_type()
        return ctype.split('/')[0]

    def get_content_subtype(self):
        """Returns the message's sub content type.

        This is the `subtype' part of the string returned by
        get_content_type().  If no slash is found in the full content type, a
        ValueError is raised.
        """
        ctype = self.get_content_type()
        return ctype.split('/')[1]

    def get_default_type(self):
        """Return the `default' content type.

        Most messages have a default content type of text/plain, except for
        messages that are subparts of multipart/digest containers.  Such
        subparts then have a default content type of message/rfc822.
        """
        return self._default_type

    def set_default_type(self, ctype):
        """Set the `default' content type.

        ctype should be either "text/plain" or "message/rfc822", although this
        is not enforced.  The default content type is not stored in the
        Content-Type: header.
        """
        self._default_type = ctype

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
                name = name.strip()
                val = val.strip()
            except ValueError:
                # Must have been a bare attribute
                name = p.strip()
                val = ''
            params.append((name, val))
        params = Utils.decode_params(params)
        return params

    def get_params(self, failobj=None, header='content-type', unquote=1):
        """Return the message's Content-Type: parameters, as a list.

        The elements of the returned list are 2-tuples of key/value pairs, as
        split on the `=' sign.  The left hand side of the `=' is the key,
        while the right hand side is the value.  If there is no `=' sign in
        the parameter the value is the empty string.  The value is always
        unquoted, unless unquote is set to a false value.

        Optional failobj is the object to return if there is no Content-Type:
        header.  Optional header is the header to search instead of
        Content-Type:.
        """
        missing = []
        params = self._get_params_preserve(missing, header)
        if params is missing:
            return failobj
        if unquote:
            return [(k, _unquotevalue(v)) for k, v in params]
        else:
            return params

    def get_param(self, param, failobj=None, header='content-type', unquote=1):
        """Return the parameter value if found in the Content-Type: header.

        Optional failobj is the object to return if there is no Content-Type:
        header.  Optional header is the header to search instead of
        Content-Type:

        Parameter keys are always compared case insensitively.  Values are
        always unquoted, unless unquote is set to a false value.
        """
        if not self.has_key(header):
            return failobj
        for k, v in self._get_params_preserve(failobj, header):
            if k.lower() == param.lower():
                if unquote:
                    return _unquotevalue(v)
                else:
                    return v
        return failobj

    def set_param(self, param, value, header='Content-Type', requote=1):
        """Set a parameter in the Content-Type: header.

        If the parameter already exists in the header, its value will be
        replaced with the new value.

        If header is Content-Type: and has not yet been defined in this
        message, it will be set to "text/plain" and the new parameter and
        value will be appended, as per RFC 2045.

        An alternate header can specified in the header argument, and
        all parameters will be quoted as appropriate unless requote is
        set to a false value.
        """
        if not self.has_key(header) and header.lower() == 'content-type':
            ctype = 'text/plain'
        else:
            ctype = self.get(header)
        if not self.get_param(param, header=header):
            if not ctype:
                ctype = _formatparam(param, value, requote)
            else:
                ctype = SEMISPACE.join(
                    [ctype, _formatparam(param, value, requote)])
        else:
            ctype = ''
            for old_param, old_value in self.get_params(header=header,
                                                        unquote=requote):
                append_param = ''
                if old_param.lower() == param.lower():
                    append_param = _formatparam(param, value, requote)
                else:
                    append_param = _formatparam(old_param, old_value, requote)
                if not ctype:
                    ctype = append_param
                else:
                    ctype = SEMISPACE.join([ctype, append_param])
        if ctype <> self.get(header):
            del self[header]
            self[header] = ctype

    def del_param(self, param, header='content-type', requote=1):
        """Remove the given parameter completely from the Content-Type header.

        The header will be re-written in place without param or its value.
        All values will be quoted as appropriate unless requote is set to a
        false value.
        """
        if not self.has_key(header):
            return
        new_ctype = ''
        for p, v in self.get_params(header, unquote=requote):
            if p.lower() <> param.lower():
                if not new_ctype:
                    new_ctype = _formatparam(p, v, requote)
                else:
                    new_ctype = SEMISPACE.join([new_ctype,
                                                _formatparam(p, v, requote)])
        if new_ctype <> self.get(header):
            del self[header]
            self[header] = new_ctype

    def set_type(self, type, header='Content-Type', requote=1):
        """Set the main type and subtype for the Content-Type: header.

        type must be a string in the form "maintype/subtype", otherwise a
        ValueError is raised.

        This method replaces the Content-Type: header, keeping all the
        parameters in place.  If requote is false, this leaves the existing
        header's quoting as is.  Otherwise, the parameters will be quoted (the
        default).

        An alternate header can be specified in the header argument.  When the
        Content-Type: header is set, we'll always also add a MIME-Version:
        header.
        """
        # BAW: should we be strict?
        if not type.count('/') == 1:
            raise ValueError
        # Set the Content-Type: you get a MIME-Version:
        if header.lower() == 'content-type':
            del self['mime-version']
            self['MIME-Version'] = '1.0'
        if not self.has_key(header):
            self[header] = type
            return
        params = self.get_params(header, unquote=requote)
        del self[header]
        self[header] = type
        # Skip the first param; it's the old type.
        for p, v in params[1:]:
            self.set_param(p, v, header, requote)

    def get_filename(self, failobj=None):
        """Return the filename associated with the payload if present.

        The filename is extracted from the Content-Disposition: header's
        `filename' parameter, and it is unquoted.
        """
        missing = []
        filename = self.get_param('filename', missing, 'content-disposition')
        if filename is missing:
            return failobj
        if isinstance(filename, TupleType):
            # It's an RFC 2231 encoded parameter
            newvalue = _unquotevalue(filename)
            return unicode(newvalue[2], newvalue[0])
        else:
            newvalue = _unquotevalue(filename.strip())
            return newvalue

    def get_boundary(self, failobj=None):
        """Return the boundary associated with the payload if present.

        The boundary is extracted from the Content-Type: header's `boundary'
        parameter, and it is unquoted.
        """
        missing = []
        boundary = self.get_param('boundary', missing)
        if boundary is missing:
            return failobj
        return _unquotevalue(boundary.strip())

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

    try:
        from email._compat22 import walk
    except SyntaxError:
        # Must be using Python 2.1
        from email._compat21 import walk

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
