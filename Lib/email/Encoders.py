# Copyright (C) 2001,2002 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Module containing encoding functions for Image.Image and Text.Text.
"""

import base64



# Helpers
try:
    from quopri import encodestring as _encodestring

    def _qencode(s):
        enc = _encodestring(s, quotetabs=1)
        # Must encode spaces, which quopri.encodestring() doesn't do
        return enc.replace(' ', '=20')
except ImportError:
    # Python 2.1 doesn't have quopri.encodestring()
    from cStringIO import StringIO
    import quopri as _quopri

    def _qencode(s):
        if not s:
            return s
        hasnewline = (s[-1] == '\n')
        infp = StringIO(s)
        outfp = StringIO()
        _quopri.encode(infp, outfp, quotetabs=1)
        # Python 2.x's encode() doesn't encode spaces even when quotetabs==1
        value = outfp.getvalue().replace(' ', '=20')
        if not hasnewline and value[-1] == '\n':
            return value[:-1]
        return value


def _bencode(s):
    # We can't quite use base64.encodestring() since it tacks on a "courtesy
    # newline".  Blech!
    if not s:
        return s
    hasnewline = (s[-1] == '\n')
    value = base64.encodestring(s)
    if not hasnewline and value[-1] == '\n':
        return value[:-1]
    return value



def encode_base64(msg):
    """Encode the message's payload in Base64.

    Also, add an appropriate Content-Transfer-Encoding header.
    """
    orig = msg.get_payload()
    encdata = _bencode(orig)
    msg.set_payload(encdata)
    msg['Content-Transfer-Encoding'] = 'base64'



def encode_quopri(msg):
    """Encode the message's payload in quoted-printable.

    Also, add an appropriate Content-Transfer-Encoding header.
    """
    orig = msg.get_payload()
    encdata = _qencode(orig)
    msg.set_payload(encdata)
    msg['Content-Transfer-Encoding'] = 'quoted-printable'



def encode_7or8bit(msg):
    """Set the Content-Transfer-Encoding header to 7bit or 8bit."""
    orig = msg.get_payload()
    if orig is None:
        # There's no payload.  For backwards compatibility we use 7bit
        msg['Content-Transfer-Encoding'] = '7bit'
        return
    # We play a trick to make this go fast.  If encoding to ASCII succeeds, we
    # know the data must be 7bit, otherwise treat it as 8bit.
    try:
        orig.encode('ascii')
    except UnicodeError:
        msg['Content-Transfer-Encoding'] = '8bit'
    else:
        msg['Content-Transfer-Encoding'] = '7bit'



def encode_noop(msg):
    """Do nothing."""
